#pragma once

#include <coroutine>

namespace dwhbll::concurrency::coroutine {
    struct uring_sqe_awaitable {
        bool await_ready() noexcept;

        void await_suspend(std::coroutine_handle<> h) noexcept;

        void await_resume() noexcept;
    };

    uring_sqe_awaitable wait_for_sqe() noexcept;
}
