#pragma once

#include <liburing.h>

#include <dwhbll/concurrency/coroutine/cancellable_base.h>
#include <dwhbll/concurrency/coroutine/reactor.h>

namespace dwhbll::concurrency::coroutine::detail {
    class sqe_awaitable : public cancellable_base {
        bool filled = false;
        std::uint32_t waiting{};
        std::array<io_uring_sqe *, 2> _sqes{};

    public:
        sqe_awaitable(std::size_t count) : waiting(count) {
            if (count > 2)
                debug::panic("Waiting on more than 2 SQEs is not currently supported!");
        }

        [[nodiscard]] constexpr bool await_ready() const noexcept {
            return io_uring_sq_space_left(reactor::get_thread_reactor()->get_uring_ptr()) >= waiting;
        }

        constexpr void await_suspend(std::coroutine_handle<> h) noexcept {
            reactor::get_thread_reactor()->enqueue_sqe_waiter(this, h, waiting);
        }

        constexpr std::array<io_uring_sqe *, 2> await_resume() noexcept {
            cancellable_base::await_resume();

            if (!filled)
                fill_sqes(reactor::get_thread_reactor()->get_uring_ptr());

            return _sqes;
        }

        constexpr void fill_sqes(io_uring* ring) {
            ASSERT(io_uring_sq_space_left(ring) >= waiting);

            for (std::size_t i = 0; i < waiting; i++)
                _sqes[i] = io_uring_get_sqe(ring);

            filled = true;
        }

        [[nodiscard]] constexpr std::size_t wanted_sqes() const {
            return waiting;
        }
    };

    constexpr sqe_awaitable wait_for_sqe(std::size_t count) noexcept {
        return {count};
    }
}
