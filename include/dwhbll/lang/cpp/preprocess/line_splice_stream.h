#pragma once

#include <list>
#include <dwhbll/lang/common.h>
#include <dwhbll/lang/common/stream.h>

namespace dwhbll::lang::cpp::preprocess {
    struct ls_token {
        bool is_line_splice;
        span spn;
        std::variant<char32_t, std::u32string> data;
    };

    struct line_splice_helper : common::stream<ls_token, 16> {
        std::unique_ptr<stream<char32_t, 16>> _source;

        cursor _head;

        std::list<ls_token> tokens;

        void refill();

    protected:
        bool has_next0() override;

        ls_token next0() override;

    public:
        explicit line_splice_helper(const files::filejar::fileid &file, std::unique_ptr<stream<char32_t, 16>> &&source);
    };
}
