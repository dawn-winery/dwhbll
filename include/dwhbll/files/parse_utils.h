#pragma once

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
}
