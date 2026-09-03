#pragma once

#include <coroutine>
#include <dwhbll/collections/ring.h>
#include <dwhbll/concurrency/coroutine/cancellable_base.h>

namespace dwhbll::concurrency::coroutine {
    class async_semaphore {
    public:
        class semaphore_awaitable;

    private:
        int32_t permits_;
        collections::Ring<std::pair<semaphore_awaitable*, std::coroutine_handle<>>> waiting;

    public:
        explicit async_semaphore(int32_t initial);

        async_semaphore(const async_semaphore&) = delete;
        async_semaphore& operator=(const async_semaphore&) = delete;

        class semaphore_awaitable : public cancellable_base {
            async_semaphore* semaphore;

            friend class async_semaphore;

            semaphore_awaitable(async_semaphore *semaphore);

        public:
            [[nodiscard]] bool await_ready() const noexcept;

            void await_suspend(std::coroutine_handle<> h);

            void await_resume() const;
        };

        class deferrable_awaitable {
            semaphore_awaitable awaitable;
            async_semaphore* semaphore;

            friend class async_semaphore;

            deferrable_awaitable(const semaphore_awaitable &awaitable,
                async_semaphore *semaphore);

        public:
            semaphore_awaitable& operator co_await();

            ~deferrable_awaitable();
        };

        semaphore_awaitable acquire() noexcept;

        [[nodiscard]] deferrable_awaitable get_with() noexcept;

        void release();
    };
}
