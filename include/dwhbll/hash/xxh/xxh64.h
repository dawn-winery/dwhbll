#pragma once

#include <dwhbll/hash/xxh/constants.h>
#include <dwhbll/hash/xxh/xxhelpers.h>
#include <dwhbll/sanify/types.h>

namespace dwhbll::hash::xxh {
    namespace detail64 {
        // scalar implementation of xxh64
        namespace scalar {
            constexpr std::array<sanify::u64, 4> initialize(sanify::u64 seed) {
                return {
                    seed + PRIME64_1 + PRIME64_2,
                    seed + PRIME64_2,
                    seed,
                    seed - PRIME64_1,
                };
            }

            constexpr sanify::u64 init_short(sanify::u64 seed) {
                return seed + PRIME64_5;
            }

            constexpr void process_stripe(std::array<sanify::u64, 4> &accs, const uint8_t* &bytes) {
                for (auto & acc : accs) {
                    acc = detail::helpers::round64(acc, detail::helpers::read_64_le(bytes));
                }
            }

            constexpr void process_stripes(std::array<sanify::u64, 4> &accs, const uint8_t* &bytes, std::size_t available_bytes) {
                while (available_bytes >= 32) {
                    process_stripe(accs, bytes);

                    available_bytes -= 32;
                }
            }

            constexpr sanify::u64 collect_accs(const std::array<sanify::u64, 4> &accs) {
                sanify::u64 acc = std::rotl(accs[0], 1) + std::rotl(accs[1], 7) + std::rotl(accs[2], 12) + std::rotl(accs[3], 18);
                acc = detail::helpers::merge_acc64(acc, accs[0]);
                acc = detail::helpers::merge_acc64(acc, accs[1]);
                acc = detail::helpers::merge_acc64(acc, accs[2]);
                acc = detail::helpers::merge_acc64(acc, accs[3]);

                return acc;
            }

            constexpr sanify::u64 consume_finalize(sanify::u64 acc, const uint8_t* &bytes, std::size_t bytes_total) {
                acc = acc + bytes_total;

                // stripes already consumed by now
                bytes_total = bytes_total % 32;

                while (bytes_total >= 8) {
                    auto lane = detail::helpers::read_64_le(bytes);
                    acc = acc ^ detail::helpers::round64(0, lane);
                    acc = std::rotl(acc, 27) * PRIME64_1;
                    acc = acc + PRIME64_4;
                    bytes_total -= 8;
                }

                while (bytes_total >= 4) {
                    auto lane = detail::helpers::read_32_le(bytes);
                    acc = acc ^ (lane * PRIME64_1);
                    acc = std::rotl(acc, 23) * PRIME64_2;
                    acc = acc + PRIME64_3;
                    bytes_total -= 4;
                }

                while (bytes_total >= 1) {
                    auto lane = *bytes;
                    acc = acc ^ (lane * PRIME64_5);
                    acc = std::rotl(acc, 11) * PRIME64_1;
                    bytes++;
                    bytes_total--;
                }

                acc = acc ^ (acc >> 33);
                acc = acc * PRIME64_2;
                acc = acc ^ (acc >> 29);
                acc = acc * PRIME64_3;
                acc = acc ^ (acc >> 32);

                return acc;
            }
        }

        constexpr sanify::u64 hash(const sanify::u8* bytes, std::size_t len, sanify::u64 seed = 0) {
            sanify::u64 acc;
            if (len < 32) [[likely]]
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
