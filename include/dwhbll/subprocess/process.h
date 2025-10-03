#pragma once

#include <expected>
#include <initializer_list>
#include <optional>
#include <string>
#include <dwhbll/subprocess/pipe_wrapper.h>

namespace dwhbll::subprocess {
    /** A return code type, the expected variant holds the case when exited normally
     *  And the unexpected variant holds the case where it exited abnormally
     */
    using returncode = std::expected<char, char>;

    class process {
        std::optional<int> stdin_;
        std::optional<int> stdout_;
        std::optional<int> stderr_;

        pid_t subprocess;
        returncode exit_code;

    public:
        process(const std::initializer_list<std::string> &args);

        ~process();

        /**
         * @brief waits until the process is done, draining anything in stdout or stderr into null.
         *
         * This is a blocking call that is a busy wait, maybe let's not make it a busy wait?
         */
        void wait_done();

        /**
         * @brief gets the pipe_wrapper that represents the stdout pipe for this process
         *
         * @note each call will return a new object however they all point to the same underlying pipe!
         *
         * @return a pipe_wrapper that represents the stdout pipe
         */
        [[nodiscard]] pipe_wrapper get_stdout_pipe() const;

        /**
         * @brief gets the pipe_wrapper that represents the stderr pipe for this process
         *
         * @note each call will return a new object however they all point to the same underlying pipe!
         *
         * @return a pipe_wrapper that represents the stderr pipe
         */
        [[nodiscard]] pipe_wrapper get_stderr_pipe() const;

        /**
         * @brief gets the pipe_wrapper that represents the stdin pipe for this process
         *
         * @note each call will return a new object however they all point to the same underlying pipe!
         *
         * @return a pipe_wrapper that represents the stdin pipe
         */
        [[nodiscard]] pipe_wrapper get_stdin_pipe() const;

        /**
         * @brief check if the process has exited, if it has set the returncode and return it
         * @return the returncode of the process, or no value if it has not yet exited.
         */
        std::optional<returncode> poll();

        /**
         * @brief waits for the process to terminate or the timeout to expire
         *
         * @param timeout the timeout to wait for in seconds.
         *
         * @throws timeout_exception if the wait has timed out
         */
        returncode wait(std::optional<long long> timeout = std::nullopt);

        /**
         * @brief communicates with the child process, sending the input and waiting for the process to terminate
         * @param input the input to send
         * @param timeout the timeout to  wait for or infinitely by default
         * @return the pair of stdout and stderr
         */
        std::pair<std::vector<char>, std::vector<char>> communicate(std::optional<std::vector<char>> input = std::nullopt, std::optional<long long> timeout = std::nullopt);

        /**
         * @brief sends a signal to the child process
         * @param signal the signal to send to the subprocess
         * @throw std::system_error if the signal was not sent sucessfully
         */
        void send_signal(int signal) const;

        /**
         * @brief sends the SIGTERM signal to the child process.
         */
        void terminate() const;

        /**
         * @brief sends a SIGKILL signal to the childprocess.
         */
        void kill() const;
    };

    process popen(const std::initializer_list<std::string> &args);
}
