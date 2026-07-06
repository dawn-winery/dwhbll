#pragma once

#include <format>
#include <list>
#include <string>
#include <unordered_map>
#include <dwhbll/utils/format.hpp>

#include <type_traits>

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

        // bs incoming
        
        template <typename T>
        decltype(auto) log_arg_to_format_arg(T&& value)
        {
            using U = std::remove_cvref_t<T>;
     
            if constexpr (std::formattable<U, char>) {
                return std::forward<T>(value);
            } 
#if __cpp_impl_reflection >= 202506L                 
            else {
                return ::dwhbll::debug::dbg(value);
            }   
#else
            else {
                static_assert(
                    false,
                    "This type is not std::formattable. "
                    "Compile this target with reflection support to log arbitrary structs."
                );
            }
#endif
        }

        template <typename T>
        using log_format_arg_t =
            std::remove_cvref_t<decltype(log_arg_to_format_arg(std::declval<T>()))>;

        template <typename... Args>
        std::string render(
            std::format_string<log_format_arg_t<Args>...> fmt,
            Args&&... args
        ) {
            auto converted = std::tuple<log_format_arg_t<Args>...>{
                log_arg_to_format_arg(std::forward<Args>(args))...
            };
        
            return std::apply(
                [&](auto&... values) {
                    return std::vformat(
                        fmt.get(),
                        std::make_format_args(values...)
                    );
                },
                converted
            );
        }
        
    }

    void setLevel(Level level);

    void setCerrLevel(Level level);

    void setWantColors(bool colors);

    void log(const std::string& msg, Level level);

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void log(std::format_string<detail::log_format_arg_t<Args>...> fmt, 
        const Level level, Args&&... args) {
        if (level < detail::defaultLevel)
            return;
        log(detail::render(fmt, std::forward<Args>(args)...),level);
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
    void fatal(std::format_string<detail::log_format_arg_t<Args>...> fmt,
        Args&&... args) {
        if (Level::FATAL < detail::defaultLevel)
            return;
        fatal(detail::render(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void critical(std::format_string<detail::log_format_arg_t<Args>...> fmt, 
        Args&&... args) {
        if (Level::CRITICAL < detail::defaultLevel)
            return;
        critical(detail::render(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void error(std::format_string<detail::log_format_arg_t<Args>...> fmt,
        Args&&... args) {
        if (Level::ERROR < detail::defaultLevel)
            return;
        error(detail::render(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void warn(std::format_string<detail::log_format_arg_t<Args>...> fmt,
        Args&&... args) {
        if (Level::WARN < detail::defaultLevel)
            return;
        warn(detail::render(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void info(std::format_string<detail::log_format_arg_t<Args>...> fmt,
        Args&&... args) {
        if (Level::INFO < detail::defaultLevel)
            return;
        info(detail::render(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void debug(std::format_string<detail::log_format_arg_t<Args>...> fmt,
        Args&&... args) {
        if (Level::DEBUG < detail::defaultLevel)
            return;
        debug(detail::render(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    void trace(std::format_string<detail::log_format_arg_t<Args>...> fmt,
        Args&&... args) {
        if (Level::TRACE < detail::defaultLevel)
            return;
        trace(detail::render(fmt, std::forward<Args>(args)...));
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
