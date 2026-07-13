#pragma once

#include <expected>
#include <utility>

#define TRY(expr)                                                          \
    ({                                                                     \
        auto&& _exp = (expr);                                              \
        if (!_exp.has_value()) {                                           \
            return std::unexpected(std::move(_exp.error()));               \
        }                                                                  \
        _exp.value();                                                      \
    })
