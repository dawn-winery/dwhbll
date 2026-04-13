#pragma once

#define TRY(expr)                                                          \
    ({                                                                     \
        auto&& _exp = (expr);                                              \
        if (!_exp.has_value()) {                                           \
            auto&& _err = _exp.error();                                    \
            return std::unexpected(std::forward<decltype(_err)>(_err));    \
        }                                                                  \
        std::forward<decltype(_exp.value())>(_exp.value());                \
    })
