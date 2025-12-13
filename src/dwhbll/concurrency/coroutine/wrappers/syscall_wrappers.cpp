#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>

#include <dwhbll/concurrency/coroutine/uring_promise.h>
#include <dwhbll/concurrency/coroutine/reactor.h>
#include <dwhbll/concurrency/coroutine/uring_sqe_awaitable.h>

#define MAKE_PROMISE \
uring_promise promise; \
co_await wait_for_sqe(); \
auto* sqe = reactor::get_thread_reactor()->get_sqe(promise);

#define SUBMIT reactor::get_thread_reactor()->submit();

namespace dwhbll::concurrency::coroutine::wrappers::calls {
    task<> nop() {
        MAKE_PROMISE

        io_uring_prep_nop(sqe);

        SUBMIT

        co_await promise;
    }

    task<int> open(const char *fptr, int flags, mode_t mode) {
        MAKE_PROMISE

        io_uring_prep_open(sqe, fptr, flags, mode);

        SUBMIT

        auto result = co_await promise;

        if (result->res < 0)
            throw exceptions::rt_exception_base("opening {} failed ({})!", fptr, strerror(-result->res));

        co_return result->res;
    }

    task<> close(int fd) {
        MAKE_PROMISE

        io_uring_prep_close(sqe, fd);

        SUBMIT

        auto result = co_await promise;

        if (result->res < 0)
            throw exceptions::rt_exception_base("closing {} failed ({})!", fd, strerror(-result->res));
    }

    task<ssize_t> read(int fd, void *buf, uint32_t count, off_t offset) {
        MAKE_PROMISE

        io_uring_prep_read(sqe, fd, buf, count, offset);

        SUBMIT

        auto result = co_await promise;

        if (result->res < 0)
            throw exceptions::rt_exception_base("reading fd {} failed ({}, {:#x} with {} at file off {})!", fd, strerror(-result->res), (std::uintptr_t)buf, count, offset);

        co_return result->res;
    }

    task<ssize_t> write(int fd, void *buf, uint32_t count, off_t offset) {
        MAKE_PROMISE

        io_uring_prep_write(sqe, fd, buf, count, offset);

        SUBMIT

        auto result = co_await promise;

        if (result->res < 0)
            throw exceptions::rt_exception_base("writing fd {} failed ({})!", fd, strerror(-result->res));

        co_return result->res;
    }

    task<int> poll(int fd, short poll_mask) {
        MAKE_PROMISE

        io_uring_prep_poll_add(sqe, fd, poll_mask);

        SUBMIT

        auto result = co_await promise;

        if (result->res < 0)
            throw exceptions::rt_exception_base("polling fd {} failed ({})!", fd, strerror(-result->res));

        co_return result->res;
    }
}
