#pragma once

#include <vector>

#include <dwhbll/files/filejar/file_mgr.h>
#include <dwhbll/lang/common/stream.h>

namespace dwhbll::lang::common {
    class file_stream : public stream<char32_t, 16> {
        std::vector<char> contents;

        bool first = true;
        std::size_t byte_head = 0;

        std::optional<char32_t> buffer;

        [[nodiscard]] bool data_left() const;

        char32_t get_char();

        std::optional<char32_t> get_next();

        void refill();

    protected:
        char32_t next0() override;

        bool has_next0() override;

    public:
        explicit file_stream(const std::vector<char>& contents);
    };
}
