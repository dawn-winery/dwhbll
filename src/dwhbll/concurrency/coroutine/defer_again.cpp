#include <dwhbll/concurrency/coroutine/defer_again.h>

#include <dwhbll/concurrency/coroutine/reactor.h>

namespace dwhbll::concurrency::coroutine {
    bool defer_again_t::await_ready() const noexcept { return false; }

    void defer_again_t::await_suspend(std::coroutine_handle<> h) const noexcept {
        reactor::get_thread_reactor()->enqueue(h);
    }

    void defer_again_t::await_resume() const noexcept {}

    defer_again_t coro::defer() {
        return defer_again_t{};
    }
}
