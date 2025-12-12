#include <dwhbll/concurrency/coroutine/sleep_task.h>

#include <dwhbll/concurrency/coroutine/reactor.h>

namespace dwhbll::concurrency::coroutine {
    bool sleep_task::await_ready() const noexcept {
        return std::chrono::steady_clock::now() >= _finish;
    }

    void sleep_task::await_suspend(std::coroutine_handle<> h) const noexcept {
        reactor::get_thread_reactor()->add_sleep_task(_finish, h);
    }

    void sleep_task::await_resume() noexcept {}
}
