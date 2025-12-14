#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>

#include <dwhbll/concurrency/coroutine/uring_promise.h>
#include <dwhbll/concurrency/coroutine/reactor.h>
#include <dwhbll/concurrency/coroutine/uring_sqe_awaitable.h>

#include <sys/socket.h>

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

        co_return result->res;
    }

    task<ssize_t> write(int fd, void *buf, uint32_t count, off_t offset) {
        MAKE_PROMISE

        io_uring_prep_write(sqe, fd, buf, count, offset);

        SUBMIT

        auto result = co_await promise;

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

    task<> connect(int fd, ::sockaddr * addr, socklen_t addrlen) {
        MAKE_PROMISE

        io_uring_prep_connect(sqe, fd, addr, addrlen);

        SUBMIT

        auto result = co_await promise;

        if (result->res < 0)
            throw exceptions::rt_exception_base("connect fd {} failed({})!", fd, strerror(-result->res));
    }

    task<ssize_t> send(int fd, const void *buf, size_t len, int flags) {
        MAKE_PROMISE

        io_uring_prep_send(sqe, fd, buf, len, flags);

        SUBMIT

        auto result = co_await promise;

        co_return result->res;
    }

    task<ssize_t> recv(int fd, void *buf, size_t len, int flags) {
        MAKE_PROMISE

        io_uring_prep_recv(sqe, fd, buf, len, flags);

        SUBMIT

        auto result = co_await promise;

        co_return result->res;
    }

    task<int> statx(int dirfd, const char *path, int flags, int mask, struct ::statx *statxbuf) {
        MAKE_PROMISE

        io_uring_prep_statx(sqe, dirfd, path, flags, mask, statxbuf);

        SUBMIT

        auto result = co_await promise;
        
        co_return result->res;
    }

    task<int> accept(int fd, sockaddr *addr, socklen_t *addrlen, int flags) {
        MAKE_PROMISE

        io_uring_prep_accept(sqe, fd, addr, addrlen, flags);

        SUBMIT

        auto result = co_await promise;

        co_return result->res;
    }
}
