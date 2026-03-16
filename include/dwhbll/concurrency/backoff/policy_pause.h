#pragma once

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace dwhbll::concurrency::backoff {
    struct PolicyPause {
    public:
        __attribute__((__always_inline__)) inline void reset() {}

        __attribute__((__always_inline__)) inline void pause() {
#if defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }
    };
}
