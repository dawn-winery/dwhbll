import std;
import dwhbll.concurrency.coroutine;
import dwhbll.debug;
import dwhbll.sanify;

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

    void cancellable_base::await_suspend(std::coroutine_handle<>) const noexcept {
        debug::panic("Unimplemented!");
    }

    void cancellable_base::await_resume() const {
        if (cancelled)
            throw cancellation_exception();
    }
}
