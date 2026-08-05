#pragma once

#include <memory>

#include <dwhbll/lang/common/stream.h>

namespace dwhbll::lang::cpp {
    /**
     * @brief CPP Raw stream, corresponding partially to end of stage 2 of
     * translation phases. Logical line splicing is performed at the same time
     * as preprocessing token generation.
     */
    class cpp_raw_stream : public common::stream<char32_t, 16> {
        std::unique_ptr<stream<char32_t, 16>> source;

        bool lf_added = false;
        bool seen_token = false;
        char32_t last = 0;

        std::optional<char32_t> _next;

        void refill();

    protected:
        char32_t next0() override;

        bool has_next0() override;

    public:
        cpp_raw_stream(std::unique_ptr<stream<char32_t, 16>> &&stream);
    };
}
