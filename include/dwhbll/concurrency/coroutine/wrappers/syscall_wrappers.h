#pragma once

#include <dwhbll/concurrency/coroutine/task.h>

namespace dwhbll::concurrency::coroutine::wrappers::calls {
    task<> nop();

    task<int> open(const char* fptr, int flags, mode_t mode = 0666);

    task<> close(int fd);

    task<ssize_t> read(int fd, void* buf, uint32_t count, off_t offset);

    task<ssize_t> write(int fd, void* buf, uint32_t count, off_t offset);

    task<int> poll(int fd, short poll_mask);
}