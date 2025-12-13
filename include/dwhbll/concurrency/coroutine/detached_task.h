#pragma once

#include <coroutine>

namespace dwhbll::concurrency::coroutine {
    struct DetachedTask {
        struct promise_type {
            DetachedTask get_return_object() noexcept;

            std::suspend_never initial_suspend() noexcept;

            struct final_awaiter {
                bool await_ready() noexcept;

                void await_suspend(std::coroutine_handle<promise_type> h) noexcept;

                void await_resume() noexcept;
            };

            final_awaiter final_suspend() noexcept;

            void unhandled_exception() noexcept;

            void return_void() noexcept;
        };

        using handle_t = std::coroutine_handle<promise_type>;

        handle_t handle;

        DetachedTask(handle_t h);
    };
}