#pragma once

#include <coroutine>
#include <dwhbll/collections/ring.h>

namespace dwhbll::concurrency::coroutine {
    class async_semaphore {
        std::int32_t permits_;
        collections::Ring<std::coroutine_handle<>> waiting;

    public:
        explicit async_semaphore(std::int32_t initial);

        async_semaphore(const async_semaphore&) = delete;
        async_semaphore& operator=(const async_semaphore&) = delete;

        struct semaphore_awaitable {
            async_semaphore* semaphore;

            bool await_ready() const noexcept;

            void await_suspend(std::coroutine_handle<> h) const;

            void await_resume() const noexcept;
        };

        struct deferrable_awaitable {
            semaphore_awaitable awaitable;
            async_semaphore* semaphore;

            semaphore_awaitable& operator co_await();

            ~deferrable_awaitable();
        };

        semaphore_awaitable acquire() noexcept;

        [[nodiscard]] deferrable_awaitable get_with() noexcept;

        void release();
    };
}
