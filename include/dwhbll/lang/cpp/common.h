#pragma once

namespace dwhbll::lang::cpp {
    static constexpr bool cpp_is_whitespace(char32_t c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f';
    }
}
