#pragma once

#include <format>
#include <list>
#include <string>
#include <unordered_map>

#define TRACE_FUNC(fmt, ...) dwhbll::console::trace("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define DEBUG_FUNC(fmt, ...) dwhbll::console::debug("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define INFO_FUNC(fmt, ...) dwhbll::console::info("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define WARN_FUNC(fmt, ...) dwhbll::console::warn("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define ERROR_FUNC(fmt, ...) dwhbll::console::error("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define CRITICAL_FUNC(fmt, ...) dwhbll::console::critical("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define FATAL_FUNC(fmt, ...) dwhbll::console::fatal("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

namespace dwhbll::console {
    enum class Level {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        // CRITICAL and FATAL are synonyms
        CRITICAL,
        // CRITICAL and FATAL are synonyms
        FATAL,
        NONE
    };

    namespace detail {
        extern Level defaultLevel;
        extern Level cerrLevel; ///< Level at which logs more important than this will be sent to stderr instead of stdout
        extern bool colors;
    }

    void setLevel(Level level);

    void setCerrLevel(Level level);

    void setWantColors(bool colors);

    void log(const std::string& msg, Level level);

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void log(const std::string& msg, const Level level, Args&&... args) {
        if (level < detail::defaultLevel)
            return;
        log(std::vformat(msg, std::make_format_args(args...)), level);
    }

    void fatal(const std::string& msg);

    void critical(const std::string& msg);

    void error(const std::string& msg);

    void warn(const std::string& msg);

    void info(const std::string& msg);

    void debug(const std::string& msg);

    void trace(const std::string& msg);

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void fatal(const std::string& msg, Args&&... args) {
        if (Level::FATAL < detail::defaultLevel)
            return;
        fatal(std::vformat(msg, std::make_format_args(args...)));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void critical(const std::string& msg, Args&&... args) {
        if (Level::CRITICAL < detail::defaultLevel)
            return;
        critical(std::vformat(msg, std::make_format_args(args...)));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void error(const std::string& msg, Args&&... args) {
        if (Level::ERROR < detail::defaultLevel)
            return;
        error(std::vformat(msg, std::make_format_args(args...)));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void warn(const std::string& msg, Args&&... args) {
        if (Level::WARN < detail::defaultLevel)
            return;
        warn(std::vformat(msg, std::make_format_args(args...)));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void info(const std::string& msg, Args&&... args) {
        if (Level::INFO < detail::defaultLevel)
            return;
        info(std::vformat(msg, std::make_format_args(args...)));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void debug(const std::string& msg, Args&&... args) {
        if (Level::DEBUG < detail::defaultLevel)
            return;
        debug(std::vformat(msg, std::make_format_args(args...)));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void trace(const std::string& msg, Args&&... args) {
        if (Level::TRACE < detail::defaultLevel)
            return;
        trace(std::vformat(msg, std::make_format_args(args...)));
    }

    /**
     * Filters a stream, e.g. processes the output data before having it written out.
     */
    class log_filter {
    public:
        virtual ~log_filter() = default;

        virtual void process(std::string& str) = 0;
    };

    /**
     * A simple filter that censors specific strings, helpful for censoring user tokens, and other potentially sensitive
     * pieces of information.
     */
    class censoring_log_filter : public log_filter {
        std::unordered_map<std::string, std::string> replacements;

    public:
        censoring_log_filter();

        explicit censoring_log_filter(const std::unordered_map<std::string, std::string> &replacements);

        ~censoring_log_filter() override = default;

        void process(std::string &str) override;

        void addBlacklist(const std::string& str);

        void addBlacklist(const std::string& str, const std::string& replacement);
    };

    extern std::list<log_filter*> log_filters;

    void addLogFilter(log_filter* filter);
}
