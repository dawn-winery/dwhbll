#pragma once

#include <format>
#include <stacktrace>
#include <vector>

namespace dwhbll::debug {
    class task_deferral;

    // I would add a skip parameter here to avoid printing internal stack
    // frames, but if I do that overload resolution shits itself
    [[noreturn]] void panic(std::string_view msg);

    [[noreturn]] void panic();

    template <typename... Args>
    requires (sizeof...(Args) != 0)
    [[noreturn]] void panic(std::string_view msg, Args&&... args) {
        panic(std::vformat(msg, std::make_format_args(args...)));
    }

    void set_header_name(const std::string &str);

    void set_trailing_string(const std::string &str);
}
