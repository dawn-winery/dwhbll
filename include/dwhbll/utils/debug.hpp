#pragma once

#include <cassert>
#include <dwhbll/console/Logging.h>

#ifdef NDEBUG
    #define ASSERT(cond, ...) ((void)0)
#else
    #define ASSERT(cond, ...) \
        ((cond) ? (void)0 : ::dwhbll::debug::assert_internal(#cond, ##__VA_ARGS__))
#endif

#define BREAKPOINT() asm("int3");

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
    if(is_being_debugged())
        BREAKPOINT();
}

[[noreturn]] inline void assert_internal(std::string_view cond) {
    panic("Assertion Failed\nCondition: {}", cond);
    if(is_being_debugged())
        BREAKPOINT();
}

} // namespace dwhbll::debug
