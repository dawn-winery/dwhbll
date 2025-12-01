#pragma once

#include <string>

namespace dwhbll::utils {
    std::string escape_non_printable(const std::string& string);

    constexpr std::string replace_all(std::string_view str, std::string_view from, std::string_view to);

    constexpr std::vector<std::string> split(std::string_view str, std::string_view sep);

    std::string escape_string(std::string_view str);
}
