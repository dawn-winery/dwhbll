#pragma once

#include <format>

#ifdef NDEBUG
    #define ASSERT(cond, ...) ((void)0)
    #define WITH_CONTEXT(fmt, ...) ((void)0)
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

#define WITH_CONTEXT(fmt, ...) auto _ = ::dwhbll::debug::task_deferral(std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#endif

#define BREAKPOINT() asm("int3")

#define timeit(stage) if (::dwhbll::debug::time __dwhbll_scope_timer__(stage); true)

#define 🦀 dbg

#define LOG_FUNC(func) \
    constexpr std::string_view __id = std::meta::identifier_of(^^func); \
    std::string __s; \
    template for (constexpr auto __e : std::define_static_array(std::meta::parameters_of(^^func))) { \
        using __T = [: std::meta::type_of(__e) :]; \
        __s += std::format("{}{} = {}\n", get_indentation(2, 5), std::meta::identifier_of(__e), dbg([: std::meta::variable_of(__e) :], 2, 5, false)); \
    } \
    trace(std::format("function {}:\n{}", __id, __s));
