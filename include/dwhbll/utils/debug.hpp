#pragma once

#include "dwhbll/utils/utils.hpp"
#include <dwhbll/utils/json.hpp>
#include <dwhbll/console/Logging.h>
#include <cassert>

#ifdef DWHBLL_REFLECTION
#include <experimental/meta>
#endif

#ifdef NDEBUG
    #define ASSERT(cond, ...) ((void)0)
#else
    #define ASSERT(cond, ...)                                                   \
        do {                                                                    \
            if(!(cond)) {                                                         \
                if(::dwhbll::debug::is_being_debugged())                        \
                    BREAKPOINT();                                               \
                else                                                            \
                    ::dwhbll::debug::assert_internal(#cond, ##__VA_ARGS__);     \
            }                                                                   \
        } while(0)
#endif

#define BREAKPOINT() asm("int3")

namespace dwhbll::debug {

// I would add a skip parameter here to avoid printing internal stack
// frames, but if I do that overload resolution shits itself
[[noreturn]] void panic(const std::string& msg);

template <typename... Args>
requires (sizeof...(Args) != 0)
[[noreturn]] void panic(const std::string& msg, Args&&... args) {
    panic(std::vformat(msg, std::make_format_args(args...)));
}

bool is_being_debugged();

template <typename... Args>
[[noreturn]] inline void assert_internal(std::string_view cond, std::string_view fmt, Args... args) {
    auto formatted_msg = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
    panic("Assertion Failed: {}\nCondition: {}", formatted_msg, cond);
}

[[noreturn]] inline void assert_internal(std::string_view cond) {
    panic("Assertion Failed\nCondition: {}", cond);
}

#ifdef DWHBLL_REFLECTION
struct DebugFmt {};
inline constexpr auto Debug = DebugFmt();

template <typename T>
requires std::is_class_v<T>
constexpr json::json debugfmt_internal(T val) {
    json::json json;
    
    constexpr auto ctx = std::meta::access_context::current();
    template for(constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx))) {
        using V = [:std::meta::type_of(m):];
        constexpr std::string_view id = std::meta::identifier_of(m);

        if constexpr (utils::has_annotation(std::meta::type_of(m), ^^DebugFmt)) {
            json[id] = debugfmt_internal<V>(val.[:m:]);
        }
        else {
            json[id] = std::format("{}", val.[:m:]);
        }
    }

    return json;
}

template <typename T>
requires std::is_class_v<T>
std::string debugfmt(T val) {
    if constexpr (!utils::has_annotation(^^T, ^^DebugFmt)) {
        throw "Missing annotation";
    }
    return debugfmt_internal(val).dump();
}
#endif

} // namespace dwhbll::debug
