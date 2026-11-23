#include <dwhbll/platform/linux_wrappers/daemon.h>

#include <fcntl.h>
#include <unistd.h>

#include <dwhbll/debug/debug.h>
#include <dwhbll/console/logging.h>

namespace dwhbll::platform::linux_wrappers {
    void daemonize(int outfd, int errfd) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull < 0)
            debug::panic("Failed to open /dev/null");
        if (dup2(devnull, STDIN_FILENO) < 0)
            debug::panic("Failed to set STDIN_FILENO");

        if (dup2(outfd, STDOUT_FILENO) < 0)
            debug::panic("Failed to set STDOUT_FILENO");
        if (dup2(errfd, STDERR_FILENO) < 0)
            debug::panic("Failed to set STDERR_FILENO");

        // force a TTY recheck
        console::checkTTY(true);

        if (daemon(0, 1) < 0)
            debug::panic("Failed to daemonize");
    }

    void daemonize(const std::filesystem::path &sout, const std::filesystem::path &serr) {
        int outfile = open(sout.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0640);
        int errfile = open(serr.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0640);

        if (outfile < 0)
            debug::panic("Failed to open file {}", sout.string());
        if (errfile < 0)
            debug::panic("Failed to open file {}", serr.string());

        daemonize(outfile, errfile);
    }

    void daemonize() {
        int devnullfd = open("/dev/null", O_WRONLY);
        daemonize(devnullfd, devnullfd);
    }
}
