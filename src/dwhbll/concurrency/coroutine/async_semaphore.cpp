#include <dwhbll/concurrency/coroutine/async_semaphore.h>

#include <dwhbll/concurrency/coroutine/reactor.h>

namespace dwhbll::concurrency::coroutine {
    async_semaphore::async_semaphore(std::int32_t initial): permits_(initial) {}

    bool async_semaphore::semaphore_awaitable::await_ready() const noexcept {
        return semaphore->permits_ > 0;
    }

    void async_semaphore::semaphore_awaitable::await_suspend(std::coroutine_handle<> h) const {
        if (semaphore->permits_ > 0) {
            semaphore->permits_--;
            reactor::get_thread_reactor()->enqueue(h);
        }

        semaphore->waiting.push_back(h);
    }

    void async_semaphore::semaphore_awaitable::await_resume() const noexcept {
    }

    async_semaphore::semaphore_awaitable& async_semaphore::deferrable_awaitable::operator co_await() {
        return awaitable;
    }

    async_semaphore::deferrable_awaitable::~deferrable_awaitable() {
        semaphore->release();
    }

    async_semaphore::semaphore_awaitable async_semaphore::acquire() noexcept {
        return semaphore_awaitable{this};
    }

    async_semaphore::deferrable_awaitable async_semaphore::get_with() noexcept {
        return deferrable_awaitable{{this}, this};
    }

    void async_semaphore::release() {
        permits_++;
        if (!waiting.empty()) {
            reactor::get_thread_reactor()->enqueue(waiting.front());
            waiting.pop_front();
        }
    }
}
