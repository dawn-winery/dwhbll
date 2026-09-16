#pragma once

#include <dwhbll/hash/xxh/constants.h>
#include <dwhbll/hash/xxh/xxhelpers.h>
#include <dwhbll/sanify/types.h>

namespace dwhbll::hash::xxh {
    namespace detail32 {
        // scalar implementation of xxh32
        namespace scalar {
            constexpr std::array<sanify::u32, 4> initialize(const sanify::u32 seed) {
                return {
                    seed + PRIME32_1 + PRIME32_2,
                    seed + PRIME32_2,
                    seed,
                    seed - PRIME32_1,
                };
            }

            constexpr sanify::u32 init_short(sanify::u32 seed) {
                return seed + PRIME32_5;
            }

            constexpr void process_stripe(std::array<sanify::u32, 4> &accs, const uint8_t* &bytes) {
                for (auto & acc : accs) {
                    acc = acc + (detail::helpers::read_32_le(bytes) * PRIME32_2);
                    acc = std::rotl(acc, 13);
                    acc = acc * PRIME32_1;
                }
            }

            constexpr void process_stripes(std::array<sanify::u32, 4> &accs, const uint8_t* &bytes, std::size_t available_bytes) {
                while (available_bytes >= 16) {
                    process_stripe(accs, bytes);

                    available_bytes -= 16;
                }
            }

            constexpr sanify::u32 collect_accs(const std::array<sanify::u32, 4> &accs) {
                return std::rotl(accs[0], 1) + std::rotl(accs[1], 7) + std::rotl(accs[2], 12) + std::rotl(accs[3], 18);
            }

            constexpr sanify::u32 consume_finalize(sanify::u32 acc, const uint8_t* &bytes, std::size_t bytes_total) {
                acc = acc + bytes_total;

                // stripes already consumed by now
                bytes_total = bytes_total % 16;

                while (bytes_total >= 4) {
                    auto lane = detail::helpers::read_32_le(bytes);
                    acc = acc + lane * PRIME32_3;
                    acc = std::rotl(acc, 17) * PRIME32_4;
                    bytes_total -= 4;
                }

                while (bytes_total >= 1) {
                    auto lane = *bytes;
                    acc = acc + lane * PRIME32_5;
                    acc = std::rotl(acc, 11) * PRIME32_1;
                    bytes++;
                    bytes_total--;
                }

                acc = acc ^ (acc >> 15);
                acc = acc * PRIME32_2;
                acc = acc ^ (acc >> 13);
                acc = acc * PRIME32_3;
                acc = acc ^ (acc >> 16);

                return acc;
            }
        }

        constexpr sanify::u32 hash(const sanify::u8* bytes, std::size_t len, sanify::u32 seed = 0) {
            sanify::u32 acc;
            if (len < 16) [[likely]]
                acc = scalar::init_short(seed);
            else {
                auto accs = scalar::initialize(seed);
                scalar::process_stripes(accs, bytes, len);
                acc = scalar::collect_accs(accs);
            }

            return scalar::consume_finalize(acc, bytes, len);
        }
    }
}
