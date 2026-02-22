#include <dwhbll/console/Logging.h>

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <dwhbll/sanify/types.hpp>
#include <dwhbll/utils/stacktrace.hpp>
#include <dwhbll/utils/string.h>
#include <dwhbll/utils/utils.hpp>

namespace dwhbll::console {
    namespace detail {
        Level defaultLevel = Level::INFO;
        Level cerrLevel = Level::ERROR;
        bool colors = true;
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
        std::stringstream ss;
        ss << (detail::colors ? tagColors[level] : "") << levelsToString[level] << msg << colorReset;
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
            str = utils::replace_all(str, from, to);
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
