#pragma once

#include <format>
#include <string>

namespace dwhbll::debug {

// I would add a skip parameter here to avoid printing internal stack
// frames, but if I do that overload resolution shits itself
[[noreturn]] void panic(const std::string& msg);

[[noreturn]] void panic();

template <typename... Args>
requires (sizeof...(Args) != 0)
[[noreturn]] void panic(const std::string& msg, Args&&... args) {
    panic(std::vformat(msg, std::make_format_args(args...)));
}

}
