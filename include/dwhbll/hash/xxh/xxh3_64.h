#pragma once

#include <dwhbll/hash/xxh/constants.h>
#include <dwhbll/hash/xxh/xxh3_common.h>
#include <dwhbll/hash/xxh/xxhelpers.h>
#include <dwhbll/sanify/types.h>

namespace dwhbll::hash::xxh {
    namespace detail3_64 {
        // scalar implementation of xxh3_64
        namespace scalar {
            // 0-16 bytes
            namespace small {
                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr sanify::u64 hash_empty(sanify::u64 seed, std::span<T> secret) {
                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data() + 56);
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 64);
                    return detail3::common::avalanche_xxh64(seed ^ secret_1 ^ secret_2);
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr sanify::u64 hash_1to3(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u32 combined = static_cast<sanify::u32>(bytes[len - 1]) |
                        (static_cast<sanify::u32>(len) << 8) |
                        (static_cast<sanify::u32>(bytes[0]) << 16) |
                        (static_cast<sanify::u32>(bytes[len >> 1]) << 24);

                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data());

                    sanify::u64 value = (((secret_1 >> 32) ^ (secret_1 & 0xFFFFFFFF)) + seed) ^ combined;
                    return detail3::common::avalanche_xxh64(value);
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr sanify::u64 hash_4to8(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u32 input_first = static_cast<sanify::u32>(bytes[0]) |
                        (static_cast<sanify::u32>(bytes[1]) << 8) |
                        (static_cast<sanify::u32>(bytes[2]) << 16) |
                        (static_cast<sanify::u32>(bytes[3]) << 24);
                    sanify::u32 input_last = static_cast<sanify::u32>(bytes[len - 4]) |
                        (static_cast<sanify::u32>(bytes[len - 3]) << 8) |
                        (static_cast<sanify::u32>(bytes[len - 2]) << 16) |
                        (static_cast<sanify::u32>(bytes[len - 1]) << 24);

                    sanify::u64 modified_seed = seed ^ (static_cast<sanify::u64>(std::byteswap(static_cast<sanify::u32>(seed))) << 32);

                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data() + 8);
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 16);

                    sanify::u64 combined = static_cast<sanify::u64>(input_last) |
                        (static_cast<sanify::u64>(input_first) << 32);

                    sanify::u64 value = ((secret_1 ^ secret_2) - modified_seed) ^ combined;
                    value = value ^ std::rotl(value, 49) ^ std::rotl(value, 24);
                    value = value * PRIME_MX2;
                    value = value ^ ((value >> 35) + len);
                    value = value * PRIME_MX2;
                    value = value ^ (value >> 28);
                    return value;
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr sanify::u64 hash_9to16(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u64 input_first = detail::helpers::bytes_to_u64_le(bytes);
                    sanify::u64 input_last = detail::helpers::bytes_to_u64_le(bytes + len - 8);

                    auto secret_1 = detail::helpers::bytes_to_u64_le(secret.data() + 24);
                    auto secret_2 = detail::helpers::bytes_to_u64_le(secret.data() + 32);
                    auto secret_3 = detail::helpers::bytes_to_u64_le(secret.data() + 40);
                    auto secret_4 = detail::helpers::bytes_to_u64_le(secret.data() + 48);

                    sanify::u64 low = ((secret_1 ^ secret_2) + seed) ^ input_first;
                    sanify::u64 high = ((secret_3 ^ secret_4) - seed) ^ input_last;
                    __u128 mul = (__u128)low * (__u128)high;
                    sanify::u64 value = len + std::byteswap(low) + high +
                        (static_cast<sanify::u64>(mul) ^ static_cast<sanify::u64>(mul >> 64));
                    return detail3::common::avalanche(value);
                }
            }

            namespace medium {
                constexpr sanify::u64 get_acc(std::size_t len) {
                    return len * PRIME64_1;
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr sanify::u64 process_17to128(sanify::u64 acc, sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u64 rounds = ((len - 1) >> 5) + 1;

                    for (ptrdiff_t i = (ptrdiff_t)rounds - 1; i >= 0; --i) {
                        auto offset_start = i * 16;
                        auto offset_end = len - i * 16 - 16;
                        acc += detail3::common::mix_step(bytes + offset_start, secret, i * 32, seed);
                        acc += detail3::common::mix_step(bytes + offset_end, secret, i * 32 + 16, seed);
                    }

                    return acc;
                }

                template <typename T>
                requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
                constexpr sanify::u64 process_129to240(sanify::u64 acc, sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                    sanify::u64 chunks = len >> 4;
                    for (size_t i = 0; i < 8; i++)
                        acc += detail3::common::mix_step(bytes + i * 16, secret,  i * 16, seed);

                    acc = detail3::common::avalanche(acc);
                    for (size_t i = 8; i < chunks; i++)
                        acc += detail3::common::mix_step(bytes + i * 16, secret, (i-8)*16 + 3, seed);
                    acc += detail3::common::mix_step(bytes + len - 16, secret, 119, seed);

                    return acc;
                }

                constexpr sanify::u64 finalize(sanify::u64 acc) {
                    return detail3::common::avalanche(acc);
                }
            }

            namespace large {
                constexpr sanify::u64 finalize(const std::array<sanify::u64, 8> &accs,
                    std::size_t len, const std::span<const sanify::u8> &secret) {
                    return detail3::common::finalize(accs, len * PRIME64_1, secret, 11);
                }
            }

            template <typename T>
            requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
            constexpr sanify::u64 hash_small(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                // 4-8, and 9-16 byte hashes happen much more often than
                // 1-3 and 0 length hashes, putting them [[likely]] reduces the
                // chances that a branch misprediction happens, reducing overall
                // percentage of time lost as latency due to branch misprediction.
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
            constexpr sanify::u64 hash_medium(sanify::u64 seed, const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
                sanify::u64 acc = medium::get_acc(len);

                if (len >= 17 && len <= 128)
                    acc = medium::process_17to128(acc, seed, bytes, len, secret);
                else if (len >= 129 && len <= 240)
                    acc = medium::process_129to240(acc, seed, bytes, len, secret);
                else
                    std::unreachable();

                return medium::finalize(acc);
            }

            constexpr sanify::u64 hash_large(const std::span<const sanify::u8> secret, const sanify::u8* bytes, std::size_t len) {
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

        constexpr sanify::u64 hash(const sanify::u8* bytes, std::size_t len) {
            if (len <= 16) [[likely]]
                return scalar::hash_small(0, bytes, len, std::span{DEFAULT_SECRET, DEFAULT_SECRET + std::size(DEFAULT_SECRET)});
            else if (len <= 240)
                return scalar::hash_medium(0, bytes, len, std::span{DEFAULT_SECRET, DEFAULT_SECRET + std::size(DEFAULT_SECRET)});
            else
                return scalar::hash_large(DEFAULT_SECRET, bytes, len);
        }

        constexpr sanify::u64 hash(const sanify::u8* bytes, std::size_t len, sanify::u64 seed) {
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
        constexpr sanify::u64 hash(const sanify::u8* bytes, std::size_t len, std::span<T> secret) {
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
        constexpr sanify::u64 hash(const sanify::u8* bytes, std::size_t len, sanify::u64 seed, std::span<T> secret) {
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