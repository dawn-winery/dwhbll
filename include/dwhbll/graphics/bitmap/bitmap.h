#pragma once

#include <cstdint>
#include <string>

namespace dwhbll::graphics::bitmap {
    class bitmap {
        int width;
        int height;
        uint8_t *pixels;

    public:
        bitmap(int width, int height);

        ~bitmap();

        [[nodiscard]] bool set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) const;

        [[nodiscard]] bool write_to_file(const std::string &filename) const;
    };
} // namespace dwhbll::graphics::bitmap
