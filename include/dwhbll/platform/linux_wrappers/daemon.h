#pragma once

#include <filesystem>

namespace dwhbll::platform::linux_wrappers {
    /**
     * @brief Daemonize this process, the process is a daemon by the time this function returns!
     * @param sout redirected path of the stdout file
     * @param serr redirected path of the stderr file
     */
    void daemonize(int outfd, int errfd);

    /**
     * @brief Daemonize this process, the process is a daemon by the time this function returns!
     * @param sout redirected path of the stdout file
     * @param serr redirected path of the stderr file
     */
    void daemonize(const std::filesystem::path &sout, const std::filesystem::path &serr);

    /**
     * @brief Daemonize this process, the process is a daemon by the time this function returns!
     *
     * This function dumps stdout and stderr directly into /dev/null
     */
    void daemonize();
}
