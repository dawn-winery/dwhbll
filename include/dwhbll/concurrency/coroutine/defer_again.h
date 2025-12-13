#pragma once

#include <coroutine>

namespace dwhbll::concurrency::coroutine {
    struct defer_again_t {
        bool await_ready() const noexcept;

        void await_suspend(std::coroutine_handle<> h) const noexcept;

        void await_resume() const noexcept;
    };

    namespace coro {
        /**
         * Reasoning for putting inside a smaller namespace:
         * - One should rarely want to use this, so it's inside a smaller
         * @return
         */
        defer_again_t defer();
    }
}
