#include <dwhbll/console/Logging.h>

#include <iostream>
#include <sstream>
#include <unordered_map>

namespace dwhbll::console {
    Level defaultLevel = Level::INFO;
    Level cerrLevel = Level::ERROR;
    bool colors = true;

    std::list<log_filter*> log_filters;

    std::unordered_map<Level, std::string> levelsToString = {
        {Level::TRACE, "TRACE"},
        {Level::DEBUG, "DEBUG"},
        {Level::INFO, "INFO "},
        {Level::WARN, "WARN "},
        {Level::ERROR, "ERROR"},
        {Level::CRITICAL, "CRIT "},
        {Level::FATAL, "FATAL"},
        {Level::NONE, "NONE "},
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

    void log(const std::string &msg, const Level level) {
        if (level < defaultLevel)
            return;
        std::stringstream ss;
        ss << (colors ? tagColors[level] : "") << "[" << levelsToString[level] << "]" << msg << colorReset;
        std::string s = ss.str();
        for (auto* filter : log_filters)
            filter->process(s);
        // TODO: don't newline and flush stream every time!
        (level >= cerrLevel ? std::cerr : std::cout) << s << std::endl;
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

    // TODO: probably move to separate header
    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
    }

    censoring_log_filter::censoring_log_filter() {}

    censoring_log_filter::censoring_log_filter(const std::unordered_map<std::string, std::string> &replacements) : replacements(replacements) {}

    void censoring_log_filter::process(std::string &str) {
        for (const auto& [from, to] : replacements)
            replaceAll(str, from, to);
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
