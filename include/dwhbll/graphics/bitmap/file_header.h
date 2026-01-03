#pragma once

#include <cstdint>

namespace dwhbll::graphics::bitmap {
    class __attribute__((packed)) file_header {
        const uint8_t h[2] = {'B', 'M'};

    public:
        uint32_t size{};
        int reserved{};
        int offset{};
    };
} // namespace dwhbll::graphics::bitmap
