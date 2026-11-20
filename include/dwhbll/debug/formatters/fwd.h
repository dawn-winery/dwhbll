#pragma once

#include <string>

// TODO: do better
namespace dwhbll::meta {
    template <typename T>
    constexpr std::string dbg(const T&, int, int, bool);
}
