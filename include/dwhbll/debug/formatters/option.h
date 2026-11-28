#pragma once

#include <format>
#include <string>

#include <dwhbll/stl_ext/option.h>
#include <dwhbll/debug/formatters/fwd.h>

template <typename T>
struct std::formatter<dwhbll::stl_ext::Option<T>, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const dwhbll::stl_ext::Option<T>& option, auto& ctx) const {
        if (option.is_some()) {
            return std::format_to(ctx.out(), "Some({})",
                dwhbll::meta::dbg(option.unwrap(), false));
        }
        return std::format_to(ctx.out(), "None");
    }
};
