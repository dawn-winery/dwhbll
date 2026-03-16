#pragma once

#include <algorithm>

#include "policy_pause.h"

namespace dwhbll::concurrency::backoff {
    class PolicyExponential {
        int spins = 1;

    public:
        __attribute__((__always_inline__)) inline void reset() {
            spins = 1;
        }

        __attribute__((__always_inline__)) inline void pause() {
            PolicyPause waiter;

            for (int i = 0; i < spins; i++)
                waiter.pause();

            spins = std::min(spins << 1, 64);
        }
    };
}
