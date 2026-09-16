#pragma once

#include <dwhbll/hash/xxh/constants.h>
#include <dwhbll/sanify/types.h>

namespace dwhbll::hash::xxh::detail::helpers {
    constexpr sanify::u64 read_64_le(const uint8_t* &bytes) {
        const std::array ba{bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]};
        auto value = std::bit_cast<uint64_t>(ba);
        if constexpr (std::endian::native == std::endian::big)
            value = std::byteswap(value);

        bytes += 8;

        return value;
    }

    constexpr sanify::u32 read_32_le(const uint8_t* &bytes) {
        const std::array ba{bytes[0], bytes[1], bytes[2], bytes[3]};
        auto value = std::bit_cast<uint32_t>(ba);
        if constexpr (std::endian::native == std::endian::big)
            value = std::byteswap(value);

        bytes += 4;

        return value;
    }

    constexpr sanify::u64 round64(sanify::u64 base, sanify::u64 accN) {
        base = base + (accN * PRIME64_2);
        base = std::rotl(base, 31);
        return base * PRIME64_1;
    }

    constexpr sanify::u64 merge_acc64(sanify::u64 base, sanify::u64 accN) {
        base = base ^ round64(0, accN);
        base = base * PRIME64_1;
        return base + PRIME64_4;
    }

    constexpr uint64_t bytes_to_u64_le(const uint8_t* bytes) {
        return static_cast<uint64_t>(bytes[0])        |
              (static_cast<uint64_t>(bytes[1]) <<  8) |
              (static_cast<uint64_t>(bytes[2]) << 16) |
              (static_cast<uint64_t>(bytes[3]) << 24) |
              (static_cast<uint64_t>(bytes[4]) << 32) |
              (static_cast<uint64_t>(bytes[5]) << 40) |
              (static_cast<uint64_t>(bytes[6]) << 48) |
              (static_cast<uint64_t>(bytes[7]) << 56);
    }

    constexpr void u64_to_bytes_le(uint8_t* bytes, const uint64_t val) {
        for (std::size_t i = 0, mask = 0xFF, shift = 0; i < 8; ++i, mask <<= 8, shift += 8)
            bytes[i] = static_cast<uint8_t>((val & mask) >> shift);
    }

    template <std::size_t N>
    constexpr auto u8_array_to_u64_le(const sanify::u8 src[N]) {
        static_assert((N % 8) == 0);

        std::array<sanify::u64, N/8> res{};
        for (std::size_t i = 0; i < N; i += 8) {
            res[i/8] = bytes_to_u64_le(src + i);
        }

        return res;
    }

    template <std::size_t N>
    constexpr auto u64_array_to_u8_le(sanify::u64* src) {
        std::array<sanify::u8, N*8> res{};
        for (std::size_t i = 0; i < N*8; i += 8)
            u64_to_bytes_le(res.data() + i, src[i/8]);

        return res;
    }

    constexpr auto derive_xxh3_secret(sanify::u64 seed) {
        std::array<sanify::u64, 24> derived_secret = u8_array_to_u64_le<std::size(DEFAULT_SECRET)>(DEFAULT_SECRET);

        for (std::size_t i = 0; i < 24; i += 2) {
            derived_secret[i] += seed;
            derived_secret[i + 1] -= seed;
        }

        return u64_array_to_u8_le<derived_secret.size()>(derived_secret.data());
    }
}
