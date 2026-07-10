#pragma once

#include <array>

#include <dwhbll/cryptography/ihash.h>

namespace dwhbll::cryptography {
    class SHA1 : ihash {
        std::uint64_t message_length; ///< Running Message Length in bytes

        std::uint8_t block[64]; ///< 512 bit block
        std::uint8_t block_head; ///< Current write index into block.

        std::uint32_t h[5]; ///< Internal state

        std::uint32_t w[80]; ///< Message schedule

        void digest_chunk();

    public:
        constexpr static std::size_t BLEN = 64;
        constexpr static std::size_t HLEN = 20;

        SHA1();

        ~SHA1() override;

        void initialize();

        void update(const std::span<const std::uint8_t> &in) override;

        void finalize(std::span<std::uint8_t> output) override;
    };
}
