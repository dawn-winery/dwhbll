#include <dwhbll/files/parse_utils.h>

#include <assert.h>
#include <dwhbll/exceptions/rt_exception_base.h>

namespace dwhbll::files {
    void ParseUtils::check_refill() {
        if (empty()) {
            refill_buffer();

            if (empty())
                throw exceptions::rt_exception_base("buffer still empty after refill!");
        }
    }

    void ParseUtils::expect(const std::string &data) {
        for (int i = 0; i < data.size(); i++) {
            check_refill();

            char c = read_u8();

            if (data[i] != c)
                throw exceptions::rt_exception_base("parse exception, unexpected character {}", c);
        }
    }

    void ParseUtils::expect(char c) {
        check_refill();

        char next = read_u8();

        if (next != c)
            throw exceptions::rt_exception_base("parse exception, unexpected character {}", next);
    }

    void ParseUtils::consume_any_whitespace() {
        while (true) {
            check_refill();

            char next = peek_u8();

            if (std::isspace(next)) {
                read_u8(); // accept token
                return;
            }
        }
    }

    std::string ParseUtils::consume_until_eol(EOLType type) {
        std::string result;

        if (type == EOLType::crlf) {
            while (true) {
                check_refill();

                char next = read_u8();

                if (next == '\r' && peek_u8() == '\n') {
                    read_u8(); // accept token
                    return result;
                }

                result.push_back(next);
            }
        }
        while (true) {
            check_refill();

            char next = read_u8();

            if (next == static_cast<char>(type))
                return result;

            result.push_back(next);
        }
    }

    std::string ParseUtils::consume_until_token(char c) {
        std::string result;

        while (true) {
            check_refill();

            const char next = read_u8();

            if (next == c)
                return result;

            result.push_back(next);
        }
    }

    std::uint64_t ParseUtils::parse_u64(int read_exactly, int radix) {
        assert(radix == 10);

        std::uint64_t buffer = 0;

        for (int i = 0; (i < read_exactly) || read_exactly == -1; i++) {
            check_refill();

            char next = peek_u8();

            if (std::isdigit(next)) {
                read_u8(); // accept the token
                buffer *= radix;
                buffer += next - '0';
            } else
                break;
        }

        return buffer;
    }
}
