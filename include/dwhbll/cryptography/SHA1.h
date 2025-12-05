#pragma once

#include "banner.h"

#include <array>

namespace dwhbll::cryptography {
    class SHA1 {
        std::uint64_t message_length; ///< Running Message Length in bytes

        std::uint8_t block[64]; ///< 512 bit block
        std::uint8_t block_head; ///< Current write index into block.

        std::uint32_t h[5]; ///< Internal state

        std::uint32_t w[80]; ///< Message schedule

        void digest_chunk();

    public:
        SHA1();

        void initialize();

        void update(const std::uint8_t* data, std::uint64_t length);

        void update(std::span<std::uint8_t> data);

        std::array<std::uint32_t, 5> finalize(const std::uint8_t* data, std::uint64_t length);

        std::array<std::uint32_t, 5> finalize();

        std::array<std::uint32_t, 5> finalize(std::span<std::uint8_t> data);
    };
}
