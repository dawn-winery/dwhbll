#include <dwhbll/concurrency/coroutine/cancellable_base.h>
#include <dwhbll/concurrency/coroutine/cancellation_exception.h>
#include <dwhbll/console/debug.h>

namespace dwhbll::concurrency::coroutine {
    void cancellable_base::cancel() noexcept {
        cancelled = true;
    }

    bool cancellable_base::is_cancelled() const noexcept {
        return cancelled;
    }

    bool cancellable_base::await_ready() const noexcept {
        debug::panic("Unimplemented!");
    }

    void cancellable_base::await_suspend(std::coroutine_handle<> h) const noexcept {
        debug::panic("Unimplemented!");
    }

    void cancellable_base::await_resume() const {
        if (cancelled)
            throw cancellation_exception();
    }
}
