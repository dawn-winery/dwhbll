#pragma once

#include <poll.h>
#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h>
#include <dwhbll/platform/wayland/display.h>

namespace dwhbll::platform::wayland {

inline concurrency::coroutine::task<> event_dispatcher(display& conn) {
    while (conn.is_alive()) {
        if (conn.prepare_poll()) {
            conn.flush();
            struct read_guard {
                display& c;
                bool done = false;
                ~read_guard() {
                    if (!done)
                        c.on_cancel();
                }
            } guard{conn};

            int res = co_await concurrency::coroutine::wrappers::calls::poll(conn.get_fd(), POLLIN | POLLHUP | POLLERR);
            if (res > 0) {
                guard.done = true;
                conn.on_readable();
            } else {
                guard.done = true;
                conn.on_cancel();
                break;
            }
        }
        conn.dispatch();
        co_await concurrency::coroutine::wrappers::calls::nop();
    }
}

} // namespace dwhbll::platform::wayland
