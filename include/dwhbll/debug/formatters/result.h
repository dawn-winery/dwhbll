#pragma once

#include <format>
#include <string>

#include <dwhbll/stl_ext/result.h>
#include <dwhbll/debug/formatters/fwd.h>

template <typename T, typename E>
struct std::formatter<dwhbll::stl_ext::Result<T, E>, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const dwhbll::stl_ext::Result<T, E>& result, auto& ctx) const {
        if (result.is_ok()) {
            return std::format_to(ctx.out(), "Ok({})", dwhbll::meta::🦀(result.unwrap(), false));
        }
        if (result.is_err()) {
            return std::format_to(ctx.out(), "Err({})", dwhbll::meta::🦀(result.unwrap_err(), false));
        }
        return std::format_to(ctx.out(), "<invalid Result>");
    }
};
