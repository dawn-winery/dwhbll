#pragma once

#include <cstdint>
#include <cstring>
#include <span>

#include "banner.h"

namespace dwhbll::cryptography {
    template <typename HASH>
    class hmac {
        constexpr static std::size_t HLEN = HASH::HASHLEN;
        constexpr static std::size_t BLEN = HASH::BLOCKLEN;

        HASH h;
        std::array<std::uint8_t, BLEN> okeydata;

    public:
        ~hmac() {
            explicit_bzero(okeydata.data(), BLEN);
        }

        void init(const std::span<const std::uint8_t>& key) {
            // prepare key
            std::array<std::uint8_t, BLEN> ikeydata;
            ikeydata.fill(0);

            if (key.size() > BLEN) {
                HASH kh;

                kh.update(key);
                kh.finalize(ikeydata);
            } else {
                std::copy(key.begin(), key.end(), ikeydata.begin());
            }
            // copy to secondary
            std::copy(ikeydata.begin(), ikeydata.end(), okeydata.begin());

            // prepare the hash operation
            // TODO: figure out what's best here
            // It's less pain this way, but
            for (auto& elem : ikeydata)
                elem ^= 0x36;
            h.update(ikeydata);

            for (auto& elem : okeydata)
                elem ^= 0x5C;

            explicit_bzero(ikeydata.data(), BLEN);
        }

        void update(const std::span<const std::uint8_t> &in) {
            h.update(in);
        }

        void finalize(std::span<std::uint8_t> output) {
            HASH out;
            std::array<std::uint8_t, HLEN> hash;

            h.finalize(hash);

            out.update(okeydata);
            out.update(hash);
            out.finalize(output);

            explicit_bzero(hash.data(), HLEN);
        }

        void reset() {
            h.reset();
            explicit_bzero(okeydata.data(), BLEN);
        }
    };
}
