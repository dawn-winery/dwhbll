#pragma once

#include <ranges>
#include <string>
#include <vector>

namespace dwhbll::utils {
    std::string escape_non_printable(const std::string& string);

    constexpr std::string replace_all(std::string_view str, std::string_view from, std::string_view to) {
        return str
            | std::views::split(from)
            | std::views::join_with(to)
            | std::ranges::to<std::string>();
    }

    constexpr std::vector<std::string> split(std::string_view str, std::string_view sep);

    std::string escape_string(std::string_view str);
}
