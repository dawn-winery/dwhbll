#pragma once

#include <format>
#include <string>

#include <dwhbll/stl_ext/cow.h>
#include <dwhbll/debug/formatters/fwd.h>


template <typename T>
struct std::formatter<dwhbll::stl_ext::cow<T>, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const dwhbll::stl_ext::cow<T>& cow, auto& ctx) const {
        const auto* ptr = cow.operator->();
        if (ptr == nullptr)
            return std::format_to(ctx.out(), "<empty cow>");

        return std::format_to(ctx.out(), "{}",dwhbll::meta::dbg(*ptr, false));
    }
};
