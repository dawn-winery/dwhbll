#pragma once

#include <dwhbll/unicode/table.h>

namespace dwhbll::lang::cpp {
    static constexpr bool cpp_is_whitespace(char32_t c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f';
    }

    static constexpr bool cpp_is_digit(char32_t c) {
        return c >= '0' && c <= '9';
    }

    static constexpr bool cpp_is_nondigit(char32_t c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    static constexpr bool cpp_is_ident_start(char32_t c) {
        return cpp_is_nondigit(c) ||
            unicode::properties::XID_Start.contains(c) ||
            unicode::properties::ID_Compat_Math_Start.contains(c);
    }

    static constexpr bool cpp_is_ident_continue(char32_t c) {
        return cpp_is_nondigit(c) ||
            cpp_is_digit(c) ||
            unicode::properties::XID_Continue.contains(c) ||
            unicode::properties::ID_Compat_Math_Continue.contains(c);
    }
}
