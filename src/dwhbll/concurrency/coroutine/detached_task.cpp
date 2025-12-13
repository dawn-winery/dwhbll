#include <dwhbll/concurrency/coroutine/detached_task.h>

#include <dwhbll/console/debug.hpp>

namespace dwhbll::concurrency::coroutine {
    DetachedTask DetachedTask::promise_type::get_return_object() noexcept {
        return {handle_t::from_promise(*this)};
    }

    std::suspend_never DetachedTask::promise_type::initial_suspend() noexcept {
        return {};
    }

    bool DetachedTask::promise_type::final_awaiter::await_ready() noexcept { return false; }

    void DetachedTask::promise_type::final_awaiter::await_suspend(std::coroutine_handle<promise_type> h) noexcept {
        h.destroy();   // 💥 self destruction
    }

    void DetachedTask::promise_type::final_awaiter::await_resume() noexcept {}

    DetachedTask::promise_type::final_awaiter DetachedTask::promise_type::final_suspend() noexcept {
        return {};
    }

    void DetachedTask::promise_type::unhandled_exception() noexcept {
        debug::panic("abandoned exception in a detached coroutine!");
    }

    void DetachedTask::promise_type::return_void() noexcept {}

    DetachedTask::DetachedTask(handle_t h) : handle(h) {}
}
