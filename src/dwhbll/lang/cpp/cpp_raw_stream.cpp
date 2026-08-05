#include <dwhbll/lang/cpp/cpp_raw_stream.h>

#include <dwhbll/stl_ext/utilities.h>

namespace dwhbll::lang::cpp {
    void cpp_raw_stream::refill() {
        if (_next.has_value())
            return;

        if (!source->has_next()) {
            if (seen_token && !lf_added) {
                _next = '\n';
                lf_added = true;
                return;
            }
            return;
        }

        seen_token = true;

        auto n = source->next();

        if (n == '\r') {
            if (source->has_next() && source->peek() == '\n')
                source->next(); // consume

            _next = '\n';
            return;
        }

        _next = n;
    }

    char32_t cpp_raw_stream::next0() {
        refill();

        if (!_next.has_value())
            debug::panic("Called without value!");

        return stl_ext::take(_next);
    }

    bool cpp_raw_stream::has_next0() {
        refill();

        return _next.has_value();
    }

    cpp_raw_stream::cpp_raw_stream(std::unique_ptr<stream<char32_t, 16>> &&stream) : source(std::move(stream)) {}
}
