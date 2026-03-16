#pragma once

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace dwhbll::concurrency::backoff {
    struct PolicyPause {
        inline void reset() {}

        inline void pause() {
#if defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }
    };
}
