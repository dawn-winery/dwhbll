#include <dwhbll/cryptography/SHA1.h>

#include <cstring>

namespace dwhbll::cryptography {
    void SHA1::digest_chunk() {
        for (std::uint8_t i = 0; i < 64; i += 4)
            w[i / 4] = (static_cast<std::uint32_t>(block[i]) << 24) & 0xFF000000 | (static_cast<std::uint32_t>(block[i + 1]) << 16) & 0x00FF0000 |
                       (static_cast<std::uint32_t>(block[i + 2]) << 8) & 0x0000FF00 | (static_cast<std::uint32_t>(block[i + 3])) & 0x000000FF;

        for (std::uint8_t i = 16; i < 80; i++)
            w[i] = std::rotl<std::uint32_t>(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

        for (std::uint8_t i = 0; i < 80; i++) {
            std::uint32_t f, k;
            if (i <= 19) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i <= 39) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i <= 59) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            const std::uint32_t temp = std::rotl<std::uint32_t>(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = std::rotl<std::uint32_t>(b, 30);
            b = a;
            a = temp;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;

        // wipe the message schedule before returning
        explicit_bzero(w, sizeof(w));
    }

    SHA1::SHA1() {
        printBanner();

        initialize();
    }

    void SHA1::initialize() {
        message_length = 0;
        block_head = 0;
        explicit_bzero(w, sizeof(w));
        explicit_bzero(block, sizeof(block));
        h[0] = 0x67452301;
        h[1] = 0xEFCDAB89;
        h[2] = 0x98BADCFE;
        h[3] = 0x10325476;
        h[4] = 0xC3D2E1F0;
    }

    void SHA1::update(const std::uint8_t *data, std::uint64_t length) {
        if (length == 0)
            return;

        if (length + block_head < 64) {
            // not enough to fill full buffer
            std::memcpy(block + block_head, data, length);
            block_head += length;
            message_length += length;
            return;
        }
        std::uint64_t copied = static_cast<std::uint64_t>(64 - block_head);
        std::memcpy(block + block_head, data, copied);
        length -= copied;
        data += copied;
        message_length += copied;
        block_head = 0;
        digest_chunk(); // process the chunk

        while (length > 64) {
            std::memcpy(block, data, 64);
            length -= 64;
            data += 64;
            message_length += 64;
            digest_chunk(); // process another chunk
        }

        explicit_bzero(block, 64); // explicitly zero the entire block
        if (length > 0) {
            std::memcpy(block, data, length);
            block_head = length;
            message_length += length;
        }
    }

    void SHA1::update(std::span<std::uint8_t> data) {
        update(data.data(), data.size());
    }

    std::array<std::uint32_t, 5> SHA1::finalize(const std::uint8_t *data, std::uint64_t length) {
        update(data, length); // first consume the whole buffer as normal, this will leave only the last < 512 bits of the message.

        if (block_head >= (64 - 8)) {
            // we need to consume this whole block

            while (block_head < 64)
                block[block_head++] = 0;

            digest_chunk();
        } else {
            block[block_head++] = 0x80;

            while (block_head < (64 - 8))
                block[block_head++] = 0;

            // this is the part block, we now have to append the 64-bit message bit length in big-endian
            message_length *= 8; // we computed this in bytes originally, we need to extend to bits.

            block[56] = (message_length >> 56) & 0xFF;
            block[57] = (message_length >> 48) & 0xFF;
            block[58] = (message_length >> 40) & 0xFF;
            block[59] = (message_length >> 32) & 0xFF;
            block[60] = (message_length >> 24) & 0xFF;
            block[61] = (message_length >> 16) & 0xFF;
            block[62] = (message_length >> 8) & 0xFF;
            block[63] = (message_length) & 0xFF;

            digest_chunk(); // consume final chunk
        }

        std::array<std::uint32_t, 5> result;

        result[0] = h[0];
        result[1] = h[1];
        result[2] = h[2];
        result[3] = h[3];
        result[4] = h[4];

        // erase all state
        initialize();

        return result;
    }

    std::array<std::uint32_t, 5> SHA1::finalize() {
        return finalize(nullptr, 0);
    }

    std::array<std::uint32_t, 5> SHA1::finalize(std::span<std::uint8_t> data) {
        return finalize(data.data(), data.size());
    }
}
