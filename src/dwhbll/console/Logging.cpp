#include <dwhbll/console/Logging.h>

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <dwhbll/sanify/types.hpp>
#include <dwhbll/utils/stacktrace.hpp>

#ifndef DWHBLL_LIBCPP
#include <stacktrace>
#include <filesystem>
#endif

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

    [[noreturn]] void panic(const std::string_view& msg, uint32_t skip) {
        std::cerr << "\n\e[1;91m========= [PANIC] =========\n";
        std::cerr << msg << "\n\n";

        #ifdef DWHBLL_LIBCPP
        using namespace dwhbll::stacktrace;
        std::vector<Entry> trace = current(skip);
        for(auto& entry : trace) {
            const auto function = entry.symbol_name.has_value() ? entry.symbol_name.value() : "???";

            std::string sourcePosition;
            if (entry.path.has_value()) {
                if(entry.line.has_value()) {
                    sourcePosition = std::format(
                        "{} at {}:{}",
                        function, entry.path.value(), entry.line.value());
                }
                else {
                    sourcePosition = std::format(
                        "{} at {}", function.data(), entry.path.value());
                }
            } else {
                sourcePosition = function;
            }

            const auto info = std::format(
                    "[{:#018x}] {}\n",
                    reinterpret_cast<std::uintptr_t>(entry.address), sourcePosition.data());
            std::cerr << (info);
        }
        #else
        std::stacktrace trace = std::stacktrace::current(skip);
        for(auto& entry : trace) {
            const auto function = entry.description().substr(0, entry.description().find("("));

            std::string sourcePosition;
            if (entry.source_file().size() > 0) {
                const auto sourcePath = std::filesystem::path(entry.source_file());
                const auto relativePath = sourcePath.lexically_relative(std::filesystem::current_path());
                const auto filename = relativePath.string().starts_with("../..") ? sourcePath : relativePath;
                sourcePosition = std::format(
                        "{} at {}:{}",
                        function.data(), filename.c_str(), entry.source_line());
            } else if (!function.empty()) {
                sourcePosition = function;
            } else if (!entry.description().empty()) {
                sourcePosition = entry.description();
            } else {
                sourcePosition = "???";
            }

            const auto info = std::format(
                    "[{:#018x}] {}\n",
                    reinterpret_cast<std::uintptr_t>(entry.native_handle()), sourcePosition.data());
            std::cerr << (info);
        }
        #endif

        exit(1);
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
