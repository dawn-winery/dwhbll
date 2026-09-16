#pragma once

#include <dwhbll/hash/xxh/constants.h>
#include <dwhbll/hash/xxh/xxh3_common.h>
#include <dwhbll/hash/xxh/xxhelpers.h>
#include <dwhbll/sanify/types.h>

namespace dwhbll::hash::xxh {
    struct u128 {
        sanify::u64 high;
        sanify::u64 low;
    };

    namespace detail3_128 {
        // scalar implementation of xxh3_128
        namespace scalar {
            // 0-16 bytes
            namespace small {
                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr u128 hash_empty(sanify::u64 seed, std::span<T> secret) {
                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data() + 64);
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 72);
                    auto secret_3 = detail::helpers::bytes_to_u64_le(secret.data() + 80);
                    auto secret_4 = detail::helpers::bytes_to_u64_le(secret.data() + 88);
                    return {
                        detail3::common::avalanche_xxh64(seed ^ secret_3 ^ secret_4),
                        detail3::common::avalanche_xxh64(seed ^ secret_1 ^ secret_2)
                    };
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr u128 hash_1to3(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u32 combined = static_cast<sanify::u32>(bytes[len - 1]) |
                        (static_cast<sanify::u32>(len) << 8) |
                        (static_cast<sanify::u32>(bytes[0]) << 16) |
                        (static_cast<sanify::u32>(bytes[len >> 1]) << 24);

                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data());
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 8);

                    sanify::u64 low = (((secret_1 >> 32) ^ (secret_1 & 0xFFFFFFFF)) + seed) ^ combined;
                    sanify::u64 high = (((secret_2 >> 32) ^ (secret_2 & 0xFFFFFFFF)) - seed) ^ std::rotl(std::byteswap(combined), 13);

                    return {
                        detail3::common::avalanche_xxh64(high),
                        detail3::common::avalanche_xxh64(low),
                    };
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr u128 hash_4to8(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u32 input_first = static_cast<sanify::u32>(bytes[0]) |
                        (static_cast<sanify::u32>(bytes[1]) << 8) |
                        (static_cast<sanify::u32>(bytes[2]) << 16) |
                        (static_cast<sanify::u32>(bytes[3]) << 24);
                    sanify::u32 input_last = static_cast<sanify::u32>(bytes[len - 4]) |
                        (static_cast<sanify::u32>(bytes[len - 3]) << 8) |
                        (static_cast<sanify::u32>(bytes[len - 2]) << 16) |
                        (static_cast<sanify::u32>(bytes[len - 1]) << 24);

                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data() + 16);
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 24);

                    sanify::u64 modified_seed = seed ^ ((sanify::u64)std::byteswap(static_cast<sanify::u32>(seed)) << 32);

                    sanify::u64 combined = static_cast<sanify::u64>(input_first) |
                        (static_cast<sanify::u64>(input_last) << 32);
                    sanify::u64 value = ((secret_1 ^ secret_2) + modified_seed) ^ combined;
                    __u128 mul = (__u128)value * (__u128)(PRIME64_1 + (len << 2));
                    auto high = static_cast<sanify::u64>(mul >> 64);
                    auto low = static_cast<sanify::u64>(mul);
                    high = high + (low << 1);
                    low = low ^ (high >> 3);
                    low = low ^ (low >> 35);
                    low = low * PRIME_MX2;
                    low = low ^ (low >> 28);
                    high = detail3::common::avalanche(high);
                    return {high, low};
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr u128 hash_9to16(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u64 input_first = detail::helpers::bytes_to_u64_le(bytes);
                    sanify::u64 input_last = detail::helpers::bytes_to_u64_le(bytes + len - 8);

                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data() + 32);
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 40);
                    auto secret_3 = detail::helpers::bytes_to_u64_le(secret.data() + 48);
                    auto secret_4 = detail::helpers::bytes_to_u64_le(secret.data() + 56);

                    sanify::u64 val1 = ((secret_1 ^ secret_2) - seed) ^ input_first ^ input_last;
                    sanify::u64 val2 = ((secret_3 ^ secret_4) + seed) ^ input_last;
                    __u128 mul = (__u128)val1 * PRIME64_1;
                    sanify::u64 low = static_cast<sanify::u64>(mul) + ((len - 1) << 54);
                    sanify::u64 high = static_cast<sanify::u64>(mul >> 64) + val2 + (val2 & 0xFFFFFFFF) * (PRIME32_2 - 1);
                    low = low ^ std::byteswap(high);

                    mul = (((__u128)high << 64) + (__u128)low) * (__u128)PRIME64_2;
                    low = static_cast<sanify::u64>(mul);
                    high = static_cast<sanify::u64>(mul >> 64);

                    return {
                        detail3::common::avalanche(high),
                        detail3::common::avalanche(low)
                    };
                }
            }

            namespace medium {
                constexpr std::array<sanify::u64, 2> get_acc(std::size_t len) {
                    return {len * PRIME64_1, 0};
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr void process_17to128(std::array<sanify::u64, 2> &accs, sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u64 rounds = ((len - 1) >> 5) + 1;

                    for (ptrdiff_t i = (ptrdiff_t)rounds - 1; i >= 0; --i) {
                        auto offset_start = i * 16;
                        auto offset_end = len - i * 16 - 16;
                        detail3::common::mix_two(accs, bytes + offset_start, bytes + offset_end, secret, i * 32, seed);
                    }
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr void process_129to240(std::array<sanify::u64, 2> &accs, sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u64 chunks = len >> 5;
                    for (size_t i = 0; i < 4; i++)
                        detail3::common::mix_two(accs, bytes + i * 32, bytes + i * 32 + 16, secret, i * 32, seed);

                    accs[0] = detail3::common::avalanche(accs[0]);
                    accs[1] = detail3::common::avalanche(accs[1]);

                    for (size_t i = 4; i < chunks; i++)
                        detail3::common::mix_two(accs, bytes + i * 32, bytes + i * 32 + 16, secret, (i - 4) * 32 + 3, seed);

                    detail3::common::mix_two(accs, bytes + len - 16, bytes + len - 32, secret, 103, static_cast<sanify::u64>(0) - seed);
                }

                constexpr u128 finalize(std::array<sanify::u64, 2> &accs, sanify::u64 seed, std::size_t len) {
                    sanify::u64 low = accs[0] + accs[1];
                    sanify::u64 high = (accs[0] * PRIME64_1) + (accs[1] * PRIME64_4) + ((len - seed) * PRIME64_2);
                    return {
                        static_cast<sanify::u64>(0) - detail3::common::avalanche(high),
                        detail3::common::avalanche(low)
                    };
                }
            }

            namespace large {
                constexpr u128 finalize(const std::array<sanify::u64, 8> &accs,
                    std::size_t len, const std::span<const sanify::u8> &secret) {
                    return {
                        detail3::common::finalize(accs, ~(len * PRIME64_2), secret, secret.size() - 75),
                        detail3::common::finalize(accs, len * PRIME64_1, secret, 11)
                    };
                }
            }

            template <typename T>
            requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
            constexpr u128 hash_small(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                // see xxh3_64.h's copy of hash_small for ordering reasoning
                if (len >= 4 && len <= 8) [[likely]]
                    return small::hash_4to8(seed, bytes, len, secret);
                else if (len >= 9 && len <= 16)
                    return small::hash_9to16(seed, bytes, len, secret);
                else if (len >= 1 && len <= 3) [[unlikely]]
                    return small::hash_1to3(seed, bytes, len, secret);
                else if (len == 0) [[unlikely]]
                    return small::hash_empty(seed, secret);
                std::unreachable();
            }

            template <typename T>
            requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
            constexpr u128 hash_medium(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                auto acc = medium::get_acc(len);

                if (len >= 17 && len <= 128)
                    medium::process_17to128(acc, seed, bytes, len, secret);
                else if (len >= 129 && len <= 240)
                    medium::process_129to240(acc, seed, bytes, len, secret);
                else
                    std::unreachable();

                return medium::finalize(acc, seed, len);
            }

            constexpr u128 hash_large(const std::span<const sanify::u8> secret, const sanify::u8* bytes, std::size_t len) {
                auto acc = detail3::common::generate_accs();

                auto stripesPerBlock = (secret.size()-64) / 8;
                auto blockSize = 64 * stripesPerBlock;

                const auto ilen = len;

                while (len > blockSize) {
                    detail3::common::round(acc, bytes, secret, stripesPerBlock);

                    bytes += blockSize;
                    len -= blockSize;
                }

                if (len < 64) [[unlikely]] {
                    // not enough data remaining, need to overlap
                    detail3::common::last_round(acc, bytes + len - 64, 64 - len, secret, len);
                } else
                    detail3::common::last_round(acc, bytes, 0, secret, len);

                return large::finalize(acc, ilen, secret);
            }
        }

        constexpr u128 hash(const sanify::u8* bytes, std::size_t len) {
            if (len <= 16)
                return scalar::hash_small(0, bytes, len, std::span{DEFAULT_SECRET, DEFAULT_SECRET + std::size(DEFAULT_SECRET)});
            else if (len <= 240)
                return scalar::hash_medium(0, bytes, len, std::span{DEFAULT_SECRET, DEFAULT_SECRET + std::size(DEFAULT_SECRET)});
            else
                return scalar::hash_large(DEFAULT_SECRET, bytes, len);
        }

        constexpr u128 hash(const sanify::u8* bytes, std::size_t len, sanify::u64 seed) {
            if (len <= 16)
                return scalar::hash_small(seed, bytes, len, std::span{DEFAULT_SECRET, DEFAULT_SECRET + std::size(DEFAULT_SECRET)});
            else if (len <= 240)
                return scalar::hash_medium(seed, bytes, len, std::span{DEFAULT_SECRET, DEFAULT_SECRET + std::size(DEFAULT_SECRET)});
            else {
                if (seed == 0)
                    return scalar::hash_large(DEFAULT_SECRET, bytes, len);
                auto secret = detail::helpers::derive_xxh3_secret(seed);
                return scalar::hash_large(secret, bytes, len);
            }
        }

        template <typename T>
        requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
        constexpr u128 hash(const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
            ASSERT(secret.size() >= 136);

            if (len <= 16) [[likely]]
                return scalar::hash_small(0, bytes, len, secret);
            else if (len <= 240)
                return scalar::hash_medium(0, bytes, len, secret);
            else
                return scalar::hash_large(secret, bytes, len);
        }

        template <typename T>
        requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
        constexpr u128 hash(const sanify::u8* bytes, std::size_t len, sanify::u64 seed, std::span<T> secret) {
            ASSERT(secret.size() >= 136);

            if (len <= 16) [[likely]]
                return scalar::hash_small(seed, bytes, len, secret);
            else if (len <= 240)
                return scalar::hash_medium(seed, bytes, len, secret);
            else
                return scalar::hash_large(secret, bytes, len);
        }
    }
}