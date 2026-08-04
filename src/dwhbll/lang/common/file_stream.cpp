#include <dwhbll/lang/common/file_stream.h>

namespace dwhbll::lang::common {
    bool file_stream::data_left() const {
        return byte_head < contents.size();
    }

    char32_t file_stream::get_char() {
        if (byte_head >= contents.size())
            debug::panic("No more bytes to read.");

        char32_t res{};
        char cur = contents[byte_head++];
        int continuation_byte_count = 0;

        switch (std::countl_one(static_cast<unsigned char>(cur))) {
        case 0:
            // UTF8 ASCII compatibility.
            return cur;
        case 1:
            // Continuation byte
            debug::panic("Extraneous UTF8 continuation byte!");
        case 2:
            // 2 byte code point
            continuation_byte_count = 1;
            res = cur & 0x1F;
            break;
        case 3:
            // 3 byte code point
            continuation_byte_count = 2;
            res = cur & 0x0F;
            break;
        case 4:
            // 4 byte code point
            continuation_byte_count = 3;
            res = cur & 0x07;
            break;
        default:
            debug::panic("Unexpected number of leading ones in UTF8 code point.");
        }

        if (byte_head + continuation_byte_count > contents.size())
            debug::panic("File ended mid UTF8 point.");

        for (int i = 0; i < continuation_byte_count; i++) {
            cur = contents.at(byte_head++);

            if (std::countl_one(static_cast<unsigned char>(cur)) != 1)
                debug::panic("Expected UTF8 continuation byte found {:#x}", static_cast<unsigned char>(cur));

            res <<= 6;
            res |= cur & 0x3F;
        }

        return res;
    }

    std::optional<char32_t> file_stream::get_next() {
        if (!first)
            return get_char();

        first = false;

        char32_t next = get_char();

        if (next == 0xFEFF) {
            if (data_left())
                return get_char();
            return std::nullopt;
        }

        return next;
    }

    void file_stream::refill() {
        if (buffer.has_value())
            return;

        if (!data_left())
            return;

        buffer = get_next();
    }

    char32_t file_stream::next0() {
        refill();

        if (!buffer.has_value())
            debug::panic("Reached end of stream already!");

        const char32_t val = buffer.value();
        buffer.reset();
        return val;
    }

    bool file_stream::has_next0() {
        refill();

        return buffer.has_value();
    }

    file_stream::file_stream(const std::vector<char> &contents) : contents(contents) {

    }
}
