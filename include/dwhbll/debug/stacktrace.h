#pragma once

#include <version>

#include <cxxabi.h>
#include <memory>
#include <string>

namespace dwhbll::debug {

inline std::string demangle(const char* name) {
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
}

} // namespace dwhbll::debug
