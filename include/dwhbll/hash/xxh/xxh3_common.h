#pragma once

#include <dwhbll/hash/xxh/constants.h>
#include <dwhbll/hash/xxh/xxhelpers.h>
#include <dwhbll/sanify/types.h>

namespace dwhbll::hash::xxh::detail3::common {
    constexpr sanify::u64 avalanche(sanify::u64 x) {
        x = x ^ (x >> 37);
        x = x * PRIME_MX1;
        x = x ^ (x >> 32);
        return x;
    }

    constexpr sanify::u64 avalanche_xxh64(sanify::u64 x) {
        x = x ^ (x >> 33);
        x = x * PRIME64_2;
        x = x ^ (x >> 29);
        x = x * PRIME64_3;
        x = x ^ (x >> 32);
        return x;
    }

    template <typename T>
    requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
    constexpr sanify::u64 mix_step(const sanify::u8* data, std::span<T> secret, const std::size_t secret_offt, const sanify::u64 seed) {
        const __u128 mul = static_cast<__u128>(detail::helpers::bytes_to_u64_le(data) ^ (detail::helpers::bytes_to_u64_le(secret.data() + secret_offt) + seed)) *
            static_cast<__u128>(detail::helpers::bytes_to_u64_le(data + 8) ^ (detail::helpers::bytes_to_u64_le(secret.data() + secret_offt + 8) - seed));
        return static_cast<sanify::u64>(mul) ^ static_cast<sanify::u64>(mul >> 64);
    }

    template <typename T>
    requires std::is_same_v<std::remove_const_t<T>, sanify::u8>
    constexpr void mix_two(std::array<sanify::u64, 2> &accs, const sanify::u8* data1,
        const sanify::u8* data2, std::span<T> secret, const std::size_t secret_offt, const sanify::u64 seed) {
        accs[0] = accs[0] + mix_step(data1, secret, secret_offt, seed);
        accs[1] = accs[1] + mix_step(data2, secret, secret_offt + 16, seed);
        accs[0] = accs[0] ^ (detail::helpers::bytes_to_u64_le(data2) + detail::helpers::bytes_to_u64_le(data2 + 8));
        accs[1] = accs[1] ^ (detail::helpers::bytes_to_u64_le(data1) + detail::helpers::bytes_to_u64_le(data1 + 8));
    }

    constexpr std::array<sanify::u64, 8> generate_accs() {
        return {
            PRIME32_3, PRIME64_1, PRIME64_2, PRIME64_3,
            PRIME64_4, PRIME32_2, PRIME64_5, PRIME32_1
        };
    }

    constexpr void accumulate(std::array<sanify::u64, 8> &accs, const sanify::u8* data,
        const std::span<const sanify::u8> &secret, std::size_t secret_offt) {
        for (std::size_t i = 0; i < 8; i++) {
            sanify::u64 value = detail::helpers::bytes_to_u64_le(data + i * 8) ^
                detail::helpers::bytes_to_u64_le(secret.data() + secret_offt + i * 8);
            accs[i ^ 1] = accs[i ^ 1] + detail::helpers::bytes_to_u64_le(data + i * 8);
            accs[i] = accs[i] + (value & 0xFFFFFFFF) * (value >> 32);
        }
    }

    constexpr void round_acc(std::array<sanify::u64, 8> &accs, const sanify::u8* data,
        const std::span<const sanify::u8> &secret, sanify::u64 stripes_per_block) {
        for (std::size_t i = 0; i < stripes_per_block; i++)
            accumulate(accs, data + i * 64, secret, i * 8);
    }

    constexpr void round_scramble(std::array<sanify::u64, 8> &accs,
        const std::span<const sanify::u8> &secret) {
        for (std::size_t i = 0; i < 8; i++) {
            accs[i] = accs[i] ^ (accs[i] >> 47);
            accs[i] = accs[i] ^ detail::helpers::bytes_to_u64_le(secret.data() + secret.size() - 64 + i * 8);
            accs[i] = accs[i] * PRIME32_1;
        }
    }

    constexpr void round(std::array<sanify::u64, 8> &accs, const sanify::u8* data,
        const std::span<const sanify::u8> &secret, sanify::u64 stripes_per_block) {
        round_acc(accs, data, secret, stripes_per_block);
        round_scramble(accs, secret);
    }

    // data_offt is provided in case there isn't enough bytes (<64 bytes) remaining,
    // in those cases the sliding window for the last stripe needs to move backwards
    // we handle this case by allowing the data ptr to actually start partially
    // forwards, len does not include area between data and data + data_offt
    constexpr void last_round(std::array<sanify::u64, 8> &accs, const sanify::u8* data,
        std::size_t data_offt, const std::span<const sanify::u8> &secret, std::size_t len) {
        std::size_t full_stripes = (len - 1) / 64;
        for (std::size_t n = 0; n < full_stripes; n++)
            accumulate(accs, data + data_offt + n * 64, secret, n * 8);

        // final accumulate of last stripe
        accumulate(accs, data + data_offt + len - 64, secret, secret.size() - 71);
    }

    constexpr sanify::u64 finalize(const std::array<sanify::u64, 8> &accs,
        sanify::u64 seed, const std::span<const sanify::u8> &secret, std::size_t secret_offt) {
        sanify::u64 result = seed;
        for (size_t i = 0; i < 4; i++) {
            __u128 mul = (__u128)(accs[i * 2] ^ detail::helpers::bytes_to_u64_le(secret.data() + secret_offt + i * 16)) *
                (__u128)(accs[i * 2 + 1] ^ detail::helpers::bytes_to_u64_le(secret.data() + secret_offt + i * 16 + 8));

            result = result + (static_cast<sanify::u64>(mul) ^ static_cast<sanify::u64>(mul >> 64));
        }

        return avalanche(result);
    }
}
