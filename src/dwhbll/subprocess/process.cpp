#include <chrono>
#include <csignal>
#include <dwhbll/console/Logging.h>
#include <dwhbll/exceptions/timeout_exception.h>
#include <dwhbll/subprocess/pipe_wrapper.h>
#include <dwhbll/subprocess/process.h>
#include <fcntl.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <dwhbll/console/debug.hpp>

namespace dwhbll::subprocess {
    process::process(const std::initializer_list<std::string> &args) {
        int child_stdin_[2];
        int child_stdout_[2];
        int child_stderr_[2];

        if (pipe2(child_stdin_, O_NONBLOCK) == -1 || pipe2(child_stdout_, O_NONBLOCK) == -1 | pipe2(child_stderr_, O_NONBLOCK) == -1)
            throw exceptions::rt_exception_base("pipe() failed");

        // stdin is reversed, we write, they read.
        stdin_ = child_stdin_[1];
        stdout_ = child_stdout_[0];
        stderr_ = child_stderr_[0];

        if ((subprocess = fork()) == -1)
            throw exceptions::rt_exception_base("fork failed");

        if (subprocess == 0) {
            dup2(child_stdin_[0], STDIN_FILENO);
            dup2(child_stdout_[1], STDOUT_FILENO);
            dup2(child_stderr_[1], STDERR_FILENO);

            // TODO: don't cary the parent's FDs into the child!

            std::vector<char*> cargs;

            cargs.reserve(args.size() + 1);

            for (auto &arg : args)
                cargs.push_back(const_cast<char*>(arg.c_str()));

            if (cargs.back() != nullptr)
                cargs.push_back(nullptr);

            execvp(cargs[0], cargs.data());
        }
    }

    process::~process() {
        if (subprocess != -1) {
            console::warn("due to filefd problems (among some other things), the subprocess will be"
                          "terminated as this object is now dead.");
            ::kill(subprocess, SIGKILL);

            waitpid(subprocess, nullptr, WUNTRACED);
        }

        if (stdin_.has_value())
            close(stdin_.value());
        if (stdout_.has_value())
            close(stdout_.value());
        if (stderr_.has_value())
            close(stderr_.value());
    }

    void process::wait_done() {
        pipe_wrapper stdout_pipe = get_stdout_pipe(), stderr_pipe = get_stderr_pipe();

        int status;
        while (waitpid(subprocess, &status, WNOHANG | WUNTRACED) == 0) {
            if (WIFEXITED(status)) {
                // exited normally
                exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                // exited non normally
                exit_code = std::unexpected(WTERMSIG(status));
            }

            // drain the pipes
            stdout_pipe.available_to_null();
            stdout_pipe.available_to_null();
        }
    }

    pipe_wrapper process::get_stdout_pipe() const {
        if (!stdout_.has_value())
            throw std::runtime_error("tried to get an stdout pipe for a process that doesn't have one!");
        return pipe_wrapper{stdout_.value()};
    }

    pipe_wrapper process::get_stderr_pipe() const {
        if (!stderr_.has_value())
            throw std::runtime_error("tried to get an stderr pipe for a process that doesn't have one!");
        return pipe_wrapper{stderr_.value()};
    }

    pipe_wrapper process::get_stdin_pipe() const {
        if (!stdin_.has_value())
            throw std::runtime_error("tried to get an stdin pipe for a process that doesn't have one!");
        return pipe_wrapper{stdin_.value()};
    }

    std::optional<returncode> process::poll() {
        int status;
        if (waitpid(subprocess, &status, WNOHANG | WUNTRACED) == 0)
            return std::nullopt;

        if (WIFEXITED(status))
            exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            exit_code = std::unexpected(WTERMSIG(status));

        return exit_code;
    }

    returncode process::wait(std::optional<long long> timeout) {
        int status;

        auto tm = std::chrono::seconds(timeout.value());

        if (timeout.has_value()) {
            auto start = std::chrono::high_resolution_clock::now();
            while (waitpid(subprocess, &status, WNOHANG | WUNTRACED) == 0) {
                if (std::chrono::high_resolution_clock::now() - start > tm)
                    throw dwhbll::exceptions::timeout_exception("timed out waiting for child process to exit.");
            }
        } else
            waitpid(subprocess, &status, WUNTRACED); // wait indefinitely

        if (WIFEXITED(status))
            exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            exit_code = std::unexpected(WTERMSIG(status));

        return exit_code;
    }

    std::pair<std::vector<char>, std::vector<char>> process::communicate(std::optional<std::vector<char>> input,
        std::optional<long long> timeout) {
        auto start = std::chrono::high_resolution_clock::now();
        auto tm = std::chrono::seconds(timeout.value_or(0));

        if (input.has_value()) {
            if (!stdin_.has_value())
                debug::panic("process::communicate given input but this process has no stdin!");
        }

        std::vector<char> stdout_output;
        std::vector<char> stderr_output;

        std::span<char> stdin_buffer;
        if (input.has_value())
            stdin_buffer = input.value();

        size_t stdin_head = 0;
        size_t stdin_remaining = stdin_buffer.size();

        std::optional<pipe_wrapper> in = stdin_.has_value() ? std::optional(get_stdin_pipe()) : std::nullopt;
        std::optional<pipe_wrapper> out = stdout_.has_value() ? std::optional(get_stdout_pipe()) : std::nullopt;
        std::optional<pipe_wrapper> err = stderr_.has_value() ? std::optional(get_stderr_pipe()) : std::nullopt;

        bool stdin_done = !stdin_.has_value(), stdout_done = !stdout_.has_value(), stderr_done = !stderr_.has_value();

        while (!stdin_done || !stdout_done || !stderr_done) {
            if (input.has_value() && !stdin_done) {
                auto write_size = in.value().ll_write(stdin_buffer);
                stdin_head += write_size;
                stdin_remaining -= write_size;
                if (stdin_head != stdin_remaining)
                    stdin_buffer = stdin_buffer.subspan(write_size);
                else
                    stdin_done = true;
            }
            if (!stdout_done) {
                auto value = out.value().ll_read(65535);
                if (!value.has_value())
                    stdout_done = true;
                auto result = value.value();
                stdout_output.insert(stdout_output.end(), result.begin(), result.end());
            }
            if (!stderr_done) {
                auto value = err.value().ll_read(65535);
                if (!value.has_value())
                    stderr_done = true;
                auto result = value.value();
                stderr_output.insert(stderr_output.end(), result.begin(), result.end());
            }
        }

        int status;
        while (waitpid(subprocess, &status, WNOHANG | WUNTRACED) == 0) {
            if (timeout.has_value() && std::chrono::high_resolution_clock::now() - start > tm)
                throw exceptions::timeout_exception("timed out waiting for child process to exit.");
        }

        return {stdout_output, stderr_output};
    }

    void process::send_signal(int signal) const {
        if (auto result = ::kill(subprocess, signal); result < 0) {
            throw std::system_error(errno, std::system_category());
        }
    }

    void process::terminate() const {
        send_signal(SIGTERM);
    }

    void process::kill() const {
        send_signal(SIGKILL);
    }

    process popen(const std::initializer_list<std::string> &args) {
        return {args};
    }
}
