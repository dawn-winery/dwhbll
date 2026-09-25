import std;
import dwhbll.concurrency.coroutine;
import dwhbll.sanify;

namespace dwhbll::concurrency::coroutine {
    async_semaphore::async_semaphore(i32 initial): permits_(initial) {}

    async_semaphore::semaphore_awaitable::semaphore_awaitable(
        async_semaphore *semaphore): semaphore(semaphore) {
    }

    bool async_semaphore::semaphore_awaitable::await_ready() const noexcept {
        return semaphore->permits_ > 0;
    }

    void async_semaphore::semaphore_awaitable::await_suspend(std::coroutine_handle<> h) {
        if (semaphore->permits_ > 0) {
            semaphore->permits_--;
            reactor::get_thread_reactor()->enqueue(this, h);
        }

        semaphore->waiting.push_back({this, h});
    }

    void async_semaphore::semaphore_awaitable::await_resume() const {
        cancellable_base::await_resume();
    }

    async_semaphore::deferrable_awaitable::deferrable_awaitable(
        const semaphore_awaitable &awaitable, async_semaphore *semaphore): awaitable(awaitable),
        semaphore(semaphore) {
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
            auto front = waiting.front();
            reactor::get_thread_reactor()->enqueue(front.first, front.second);
            waiting.pop_front();
        }
    }
}
