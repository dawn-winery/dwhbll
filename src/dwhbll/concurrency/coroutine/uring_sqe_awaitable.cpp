#include <dwhbll/concurrency/coroutine/reactor.h>
#include <dwhbll/concurrency/coroutine/uring_sqe_awaitable.h>

namespace dwhbll::concurrency::coroutine {
    bool uring_sqe_awaitable::await_ready() noexcept {
        return io_uring_sq_space_left(reactor::get_thread_reactor()->get_uring_ptr()) > 0;
    }

    void uring_sqe_awaitable::await_suspend(std::coroutine_handle<> h) noexcept {
        // Defer until reactor frees SQ space
        reactor::get_thread_reactor()->enqueue_sqe_waiter(h);
    }

    void uring_sqe_awaitable::await_resume() noexcept {}

    uring_sqe_awaitable wait_for_sqe() noexcept {
        return uring_sqe_awaitable();
    }
}
