#include <liburing.h>

import std;
import dwhbll.concurrency.coroutine;
import dwhbll.sanify;

namespace dwhbll::concurrency::coroutine {
    bool uring_promise::await_ready() const noexcept {
        return false;
    }

    void uring_promise::await_suspend(std::coroutine_handle<> h) noexcept {
        waiter = h;
    }

    io_uring_cqe * uring_promise::await_resume() const {
        cancellable_base::await_resume();

        return cqe;
    }
}
