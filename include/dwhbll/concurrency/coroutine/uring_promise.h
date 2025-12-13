#pragma once

#include <coroutine>
#include <liburing/io_uring.h>

namespace dwhbll::concurrency::coroutine {
    /**
     * @brief an awaitable for the sole purpose of waiting for the associated uring completion.
     */
    struct uring_promise {
        io_uring_cqe* cqe = nullptr;
        std::coroutine_handle<> waiter;

        bool await_ready() const noexcept;

        void await_suspend(std::coroutine_handle<> h) noexcept;

        io_uring_cqe* await_resume() noexcept;
    };
}
