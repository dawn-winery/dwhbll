module;

#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/concurrency/coroutine/detached_task.h>
#include <dwhbll/concurrency/coroutine/sleep_task.h>
#include <dwhbll/concurrency/coroutine/cancellable_base.h>
#include <dwhbll/concurrency/coroutine/cancellation_exception.h>
#include <dwhbll/concurrency/coroutine/uring_promise.h>
#include <dwhbll/concurrency/coroutine/uring_sqe_awaitable.h>
#include <dwhbll/concurrency/coroutine/async_semaphore.h>
#include <dwhbll/concurrency/coroutine/reactor.h>
#include <dwhbll/concurrency/coroutine/defer_again.h>
#include <dwhbll/concurrency/coroutine/wrappers/file.h>
#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>

export module dwhbll.concurrency.coroutine;

export namespace dwhbll::concurrency::coroutine {
    using dwhbll::concurrency::coroutine::task;
    using dwhbll::concurrency::coroutine::DetachedTask;
    using dwhbll::concurrency::coroutine::sleep_task;
    using dwhbll::concurrency::coroutine::sleep_for;
    using dwhbll::concurrency::coroutine::cancellable_base;
    using dwhbll::concurrency::coroutine::cancellation_exception;
    using dwhbll::concurrency::coroutine::uring_promise;
    using dwhbll::concurrency::coroutine::uring_sqe_awaitable;
    using dwhbll::concurrency::coroutine::wait_for_sqe;
    using dwhbll::concurrency::coroutine::async_semaphore;
    using dwhbll::concurrency::coroutine::reactor;
    using dwhbll::concurrency::coroutine::defer_again_t;

    namespace wrappers {
        using dwhbll::concurrency::coroutine::wrappers::file;
        namespace calls {
            using dwhbll::concurrency::coroutine::wrappers::calls::nop;
            using dwhbll::concurrency::coroutine::wrappers::calls::open;
            using dwhbll::concurrency::coroutine::wrappers::calls::close;
            using dwhbll::concurrency::coroutine::wrappers::calls::read;
            using dwhbll::concurrency::coroutine::wrappers::calls::write;
            using dwhbll::concurrency::coroutine::wrappers::calls::poll;
            using dwhbll::concurrency::coroutine::wrappers::calls::connect;
            using dwhbll::concurrency::coroutine::wrappers::calls::send;
            using dwhbll::concurrency::coroutine::wrappers::calls::recv;
            using dwhbll::concurrency::coroutine::wrappers::calls::statx;
            using dwhbll::concurrency::coroutine::wrappers::calls::accept;
        }
    }

    namespace coro {
        using dwhbll::concurrency::coroutine::coro::defer;
    }
}
