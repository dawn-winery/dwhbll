#include <cstring>

import std;
import dwhbll.cryptography;
import dwhbll.sanify;

namespace dwhbll::cryptography::arc4 {
    u8 arc4::keystream_next() {
        ++keygen_i;
        keygen_j += S[keygen_i];

        std::swap(S[keygen_i], S[keygen_j]);

        u8 t = S[keygen_i] + S[keygen_j];

        return S[t];
    }

    arc4::arc4(std::vector<u8> const &key) {
        for (int i = 0; i < 256; i++)
            S[i] = i;

        if (key.size() > 256 || key.empty())
            throw std::range_error("ARC4 Key size not within range");

        u8 j = 0;
        for (int i = 0; i < 256; i++) {
            j = j + S[i] + key[i % key.size()];
            std::swap(S[j], S[i]);
        }
    }

    arc4::~arc4() {
        explicit_bzero(S, 256);
        explicit_bzero(&keygen_i, 1);
        explicit_bzero(&keygen_j, 1);
    }

    std::vector<u8> arc4::crypt(const std::vector<u8> &data) {
        std::vector<u8> result;
        result.resize(data.size());

        for (std::size_t i = 0; i < data.size(); i++)
            result[i] = data[i] ^ keystream_next();

        return result;
    }

    void arc4::crypt_inplace(std::vector<u8> &data) {
        crypt_inplace(data.data(), data.size());
    }

    void arc4::crypt_inplace(u8 *data, std::size_t size) {
        for (std::size_t i = 0; i < size; i++)
            data[i] ^= keystream_next();
    }

    void arc4::drop(std::size_t count) {
        for (std::size_t i = 0; i < count; i++)
            keystream_next();
    }
}
