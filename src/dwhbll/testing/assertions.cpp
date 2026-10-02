#include <cstring>
#include <dwhbll/testing/assertions.h>
#include <dwhbll/debug/debug.h>

#include <fcntl.h>
#include <regex>
#include <sys/wait.h>
#include <unistd.h>

namespace dwhbll::test {

namespace detail {

void result::add_failure(std::string msg, std::source_location loc) {
    failures_.push_back({std::move(msg), loc});
}

result* current_result = nullptr;

void report_failure(std::string msg, std::source_location loc) {
    ASSERT(current_result);
    current_result->add_failure(std::move(msg), loc);
}

} // namespace detail

bool expect(bool cond, std::string_view msg, std::source_location loc) {
    if (!cond)
        detail::report_failure(msg.empty() ? std::string("expect failed")
                                           : std::string(msg), loc);
    return cond;
}

namespace {

struct death_info {
    bool died = false;
    int signal_num = 0;
    int exit_code = 0;
    std::string captured_output;
};

death_info run_child(const std::function<void()>& fn) {
    int pipe_fds[2];
    if (pipe(pipe_fds) != 0)
        return {.died = false, .captured_output = "pipe failed: " + std::string(strerror(errno))};

    pid_t pid = fork();
    if (pid < 0) {
        std::string reason = "fork failed: " + std::string(strerror(errno));
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return {.died = false, .captured_output = reason};
    }

    if (pid == 0) {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDOUT_FILENO);
        dup2(pipe_fds[1], STDERR_FILENO);
        close(pipe_fds[1]);

        try {
            fn();
        } catch (...) {
            _exit(128 + 6);
        }
        _exit(0);
    }

    close(pipe_fds[1]);

    std::string output;
    char buffer[512];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fds[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        output.append(buffer, bytes_read);
    }
    close(pipe_fds[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    death_info res;
    res.captured_output = std::move(output);

    if (WIFSIGNALED(status)) {
        res.signal_num = WTERMSIG(status);
        res.died = true;
    } else if (WIFEXITED(status)) {
        res.exit_code = WEXITSTATUS(status);
        res.died = res.exit_code != 0;
    }

    return res;
}

} // namespace

bool expect_death(std::function<void()> fn, int code,
                  std::string_view msg, std::source_location loc) {
    auto info = run_child(fn);

    if (!info.died) {
        std::string fail_msg = std::format("EXPECT_DEATH failed: process did not terminate (expr: {})", msg);
        detail::report_failure(fail_msg, loc);
        return false;
    }

    // Kind of jank, but whatever
    bool match = (info.signal_num == code) || (info.exit_code == code);

    if (!match) {
        std::string fail_msg = std::format("EXPECT_DEATH failed: died with signal {} / exit code {}, expected signal or status {} (expr: {})",
                                           info.signal_num, info.exit_code, code, msg);
        detail::report_failure(fail_msg, loc);
        return false;
    }

    return true;
}

bool expect_death(std::function<void()> fn, std::string_view pattern,
                  std::string_view msg, std::source_location loc) {
    auto info = run_child(fn);

    if (!info.died) {
        std::string fail_msg = std::format("EXPECT_DEATH failed: process did not terminate (expr: {})", msg);
        detail::report_failure(fail_msg, loc);
        return false;
    }

    bool matches = pattern.empty();
    if (!matches) {
        if (info.captured_output.find(pattern) != std::string::npos) {
            matches = true;
        } else {
            std::regex re{std::string(pattern)};
            matches = std::regex_search(info.captured_output, re);
        }
    }

    if (!matches) {
        std::string fail_msg = std::format("EXPECT_DEATH failed: did not match pattern '{}'\nexpr: {}\nActual output:\n{}",
                                           pattern, msg, info.captured_output);
        detail::report_failure(fail_msg, loc);
        return false;
    }

    return true;
}

} // namespace dwhbll::test
