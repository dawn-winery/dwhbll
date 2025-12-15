#pragma once

#include <dwhbll/concurrency/coroutine/task.h>
#include <linux/stat.h>
#include <sys/socket.h>

namespace dwhbll::concurrency::coroutine::wrappers::calls {
    task<> nop();

    task<int> open(const char* fptr, int flags, mode_t mode = 0666);

    task<> close(int fd);

    task<ssize_t> read(int fd, void* buf, uint32_t count, off_t offset);

    task<ssize_t> write(int fd, void* buf, uint32_t count, off_t offset);

    task<int> poll(int fd, short poll_mask);

    task<> connect(int fd, ::sockaddr* addr, socklen_t addrlen);

    task<ssize_t> send(int fd, const void* buf, size_t len, int flags);

    task<ssize_t> recv(int fd, void* buf, size_t len, int flags);

    task<int> statx(int dirfd, const char* path, int flags, int mask, statx* statxbuf);

    task<int> accept(int fd, sockaddr* addr, socklen_t* addrlen, int flags);
}
