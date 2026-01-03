#pragma once

namespace dwhbll::graphics::bitmap {
    struct __attribute__((packed)) info_header {
        uint32_t size = 40;
        int width{};
        int height{};
        short planes = 1;
        short bits_pp = 24;
        int compression{};
        int h_res = 2400;
        int v_res = 2400;
        int colors = 0;
        int imp_colors = 0;
    };
} // namespace dwhbll::graphics::bitmap
