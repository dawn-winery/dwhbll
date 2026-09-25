import std;
import dwhbll.concurrency.coroutine;
import dwhbll.sanify;

namespace dwhbll::concurrency::coroutine {
    bool defer_again_t::await_ready() const noexcept { return false; }

    void defer_again_t::await_suspend(std::coroutine_handle<> h) noexcept {
        reactor::get_thread_reactor()->enqueue(this, h);
    }

    void defer_again_t::await_resume() const noexcept {
        cancellable_base::await_resume();
    }

    namespace coro {
        defer_again_t defer() {
            return defer_again_t{};
        }
    }
}
