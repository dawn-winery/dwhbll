#pragma once

#include <format>
#include <utility>

#ifdef NDEBUG
    #define ASSERT(cond, ...) ((void)0)
#else
#define ASSERT(cond, ...)                                                   \
    do {                                                                    \
        if(!(cond)) {                                                       \
            if(::dwhbll::debug::is_being_debugged())                        \
                BREAKPOINT();                                               \
            else                                                            \
                ::dwhbll::debug::assert_internal(#cond, ##__VA_ARGS__);     \
        }                                                                   \
    } while(0)
#endif

#define BREAKPOINT() asm("int3")

namespace dwhbll::debug {

#ifdef NDEBUG
#define WITH_CONTEXT(fmt, ...) ((void)0)
#else
class task_deferral {
    std::string name;

public:
    task_deferral(const std::string &name);

    ~task_deferral();

    const std::string& get_name() const;
};

// TODO: to minimize cost we could probably just store FMT args and format on demand.
#define WITH_CONTEXT(fmt, ...) auto _ = ::dwhbll::debug::task_deferral(std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#endif

// I would add a skip parameter here to avoid printing internal stack
// frames, but if I do that overload resolution shits itself
[[noreturn]] void panic(const std::string& msg);

[[noreturn]] void panic();

template <typename... Args>
requires (sizeof...(Args) != 0)
[[noreturn]] void panic(const std::string& msg, Args&&... args) {
    panic(std::vformat(msg, std::make_format_args(args...)));
}

template <typename... Args>
void cond_assert(bool condition, const std::string& msg, Args&&... args) {
    if (!condition)
        panic(std::vformat(msg, std::make_format_args(args...)));
}

void cond_assert(bool condition);

bool is_being_debugged();

template <typename... Args>
[[noreturn]] inline void assert_internal(std::string_view cond, std::string_view fmt, Args... args) {
    auto formatted_msg = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
    panic("Assertion Failed: {}\nCondition: {}", formatted_msg, cond);
}

[[noreturn]] inline void assert_internal(std::string_view cond) {
    panic("Assertion Failed\nCondition: {}", cond);
}

[[noreturn]] inline void unreachable() {
#ifdef NDEBUG
    std::unreachable();
#else
    panic("Reached unreachable statement");
#endif
}

[[noreturn]] inline void todo() {
    panic("Not implemented!");
}

} // namespace dwhbll::debug
