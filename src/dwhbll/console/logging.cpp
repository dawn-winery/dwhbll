#include <dwhbll/console/logging.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <unistd.h>

#include <dwhbll/sanify/types.h>
#include <dwhbll/stl_ext/option.h>
#include <dwhbll/stl_ext/string.h>


namespace dwhbll::console {
    namespace detail {
        Level defaultLevel = Level::INFO;
        Level cerrLevel = Level::ERROR;
        bool colors = true;

        stl_ext::Option<bool> stdout_is_tty = stl_ext::None();
        stl_ext::Option<bool> stderr_is_tty = stl_ext::None();
    }

    std::list<log_filter*> log_filters;

    std::unordered_map<Level, std::string> levelsToString = {
        {Level::TRACE, "[TRACE] "},
        {Level::DEBUG, "[DEBUG] "},
        {Level::INFO, "[INFO]  "},
        {Level::WARN, "[WARN]  "},
        {Level::ERROR, "[ERROR] "},
        {Level::CRITICAL, "[CRIT]  "},
        {Level::FATAL, "[FATAL] "},
        {Level::NONE, "[NONE]  "},
    };

    std::unordered_map<Level, std::string> tagColors = {
        {Level::TRACE, ""},
        {Level::DEBUG, ""},
        {Level::INFO, ""},
        {Level::WARN, "\033[93m"},
        {Level::ERROR, "\033[91m"},
        {Level::CRITICAL, "\033[41m\033[97m"},
        {Level::FATAL, "\033[41m\033[97m"},
        {Level::NONE, ""},
    };

    const std::string colorReset = "\033[0m";

    void checkTTY(bool force) {
        bool check_stdout = force || detail::stdout_is_tty.is_none();
        bool check_stderr = force || detail::stderr_is_tty.is_none();

        auto check = [](stl_ext::Option<bool> &res, int fd) {
            auto r = isatty(fd);
            if (r == 1)
                res = stl_ext::Some(true);
            else if (errno == ENOTTY || errno == EINVAL) {
                // EINVAL is a violation of posix but some kernels do it anyway :xdd:
                res = stl_ext::Some(false);
            } else {
                std::cerr << tagColors[Level::FATAL] << std::format(
                    "[FATAL] Failed to check if fd {} is TTY (errno: {}, {}), assuming is not TTY",
                    fd, errno, strerror(errno)) << std::endl;
                res = stl_ext::Some(false);
            }
        };

        if (check_stdout)
            check(detail::stdout_is_tty, STDOUT_FILENO);

        if (check_stderr)
            check(detail::stderr_is_tty, STDERR_FILENO);
    }

    void setLevel(Level level) {
        detail::defaultLevel = level;
    }

    void setCerrLevel(Level level) {
        detail::cerrLevel = level;
    }

    void setWantColors(bool colors) {
        detail::colors = colors;
    }

    void log(const std::string &msg, const Level level) {
        if (level < detail::defaultLevel)
            return;

        checkTTY();

        bool colors = (level >= detail::cerrLevel ? detail::stderr_is_tty : detail::stdout_is_tty).unwrap();
        colors &= detail::colors;

        std::stringstream ss;
        ss << (colors ? tagColors[level] : "") << levelsToString[level] << msg << (colors ? colorReset : "");
        std::string s = ss.str();
        for (auto* filter : log_filters)
            filter->process(s);
        // TODO: don't newline and flush stream every time!
        (level >= detail::cerrLevel ? std::cerr : std::cout) << s << std::endl;
    }

    void fatal(const std::string &msg) {
        log(msg, Level::FATAL);
    }

    void critical(const std::string &msg) {
        log(msg, Level::CRITICAL);
    }

    void error(const std::string &msg) {
        log(msg, Level::ERROR);
    }

    void warn(const std::string &msg) {
        log(msg, Level::WARN);
    }

    void info(const std::string &msg) {
        log(msg, Level::INFO);
    }

    void debug(const std::string &msg) {
        log(msg, Level::DEBUG);
    }

    void trace(const std::string &msg) {
        log(msg, Level::TRACE);
    }

    censoring_log_filter::censoring_log_filter() {}

    censoring_log_filter::censoring_log_filter(const std::unordered_map<std::string, std::string> &replacements) : replacements(replacements) {}

    void censoring_log_filter::process(std::string &str) {
        for (const auto& [from, to] : replacements)
            str = stl_ext::replace_all(str, from, to);
    }

    void censoring_log_filter::addBlacklist(const std::string &str) {
        replacements[str] = "[CENSORED]";
    }

    void censoring_log_filter::addBlacklist(const std::string &str, const std::string &replacement) {
        replacements[str] = replacement;
    }

    void addLogFilter(log_filter *filter) {
        log_filters.push_back(filter);
    }
}
