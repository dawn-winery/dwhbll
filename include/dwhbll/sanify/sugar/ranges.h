#pragma once

#include <cstdint>

namespace dwhbll::sanify::sugar {
    namespace __detail {
        struct range_iterable {
            const std::int64_t begin;
            const std::int64_t end;
        };
    }

    constexpr __detail::range_iterable range(const std::int64_t begin, const std::int64_t end) {
        return __detail::range_iterable{begin, end};
    }

    constexpr __detail::range_iterable range(const std::int64_t end) {
        return __detail::range_iterable{0, end};
    }
}

#ifdef DWHBLL_SUGAR_EXPORT
using namespace dwhbll::sanify::sugar;
#endif
