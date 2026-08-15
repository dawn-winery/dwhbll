#pragma once

#include <span>

#include <dwhbll/collections/memory_buffer.h>

namespace dwhbll::files {
    enum class EOLType {
        crlf,
        cr = '\r',
        lf = '\n'
    };
    class ParseUtils : public collections::MemBuf {
    public:
        using MemBuf::MemBuf;

        void check_refill();

        void expect(const std::string& data);

        void expect(char c);

        void consume_any_whitespace();

        std::string consume_until_eol(EOLType type = EOLType::lf);

        std::string consume_until_token(char c);

        std::uint64_t parse_u64(int read_exactly = -1, int radix = 10);
    };

    constexpr uint8_t read_u8(std::span<uint8_t> &file) {
        if (file.empty())
            debug::panic("No data in buffer.");

        uint8_t res = file[0];
        file = file.subspan(1);
        return res;
    }

    constexpr uint16_t read_u16_le(std::span<uint8_t> &data) {
        auto first = read_u8(data);
        auto second = read_u8(data);

        return static_cast<uint16_t>(first) | (static_cast<uint16_t>(second) << 8);
    }

    constexpr uint32_t read_u32_le(std::span<uint8_t> &data) {
        auto first = read_u16_le(data);
        auto second = read_u16_le(data);

        return static_cast<uint32_t>(first) | (static_cast<uint32_t>(second) << 16);
    }

    constexpr uint64_t read_u64_le(std::span<uint8_t> &data) {
        auto first = read_u32_le(data);
        auto second = read_u32_le(data);

        return static_cast<uint64_t>(first) | (static_cast<uint64_t>(second) << 32);
    }
}
