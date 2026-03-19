#pragma once

#include <cstdint>
#include <vector>

#include <dwhbll/cryptography/banner.h>

namespace dwhbll::cryptography::arc4 {
    class arc4 {
        std::uint8_t S[256];
        std::uint8_t keygen_i{};
        std::uint8_t keygen_j{};

        std::uint8_t keystream_next();

    public:
        explicit arc4(std::vector<std::uint8_t> const &key);

        ~arc4();

        std::vector<std::uint8_t> crypt(const std::vector<std::uint8_t>& data);

        void crypt_inplace(std::vector<std::uint8_t>& data);

        void crypt_inplace(std::uint8_t* data, std::size_t size);

        void drop(std::size_t count);
    };
}
