import std;
import dwhbll.concurrency.coroutine;
import dwhbll.sanify;

namespace dwhbll::concurrency::coroutine {
    bool sleep_task::await_ready() const noexcept {
        return std::chrono::steady_clock::now() >= _finish;
    }

    void sleep_task::await_suspend(std::coroutine_handle<> h) noexcept {
        reactor::get_thread_reactor()->add_sleep_task(_finish, this, h);
    }

    void sleep_task::await_resume() const {
        cancellable_base::await_resume();
    }
}
