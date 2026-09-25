#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

import std;
import dwhbll.concurrency;
import dwhbll.console;
import dwhbll.exceptions;
import dwhbll.sanify;

namespace dwhbll::concurrency {
    spinlock::~spinlock() {
#ifdef DWHBLL_HARDEN_EXPENSIVE
        if (_lock.test()) {
            throw dwhbll::exceptions::concurrency_exception("Spinlock was destroyed with lock still held!");
        }
#elifdef DWHBLL_HARDEN
        if (_lock.test()) {
            console::warn("Spinlock was destroyed with lock still held!");
        }
#endif
    }

    dwhbll::sanify::deferred spinlock::lock() {
        while (_lock.test_and_set(std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }

        return sanify::deferred([this] {
            this->_lock.clear(std::memory_order_release);
        });
    }
}
