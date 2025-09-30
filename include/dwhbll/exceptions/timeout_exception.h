#pragma once

#include <stdexcept>

namespace dwhbll::exceptions {
    class timeout_exception : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
}
