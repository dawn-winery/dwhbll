#pragma once

#include <chrono>
#include <coroutine>
#include <dwhbll/collections/ring.h>
#include <dwhbll/collections/sorted_linked_list.h>
#include <dwhbll/concurrency/coroutine/task.h>
#include <liburing.h>

namespace dwhbll::concurrency::coroutine {
    struct uring_promise;
    class reactor;

    namespace detail {
        extern thread_local reactor* live_reactor;
    }

    class reactor {
        struct timer_task {
            std::chrono::steady_clock::time_point time;
            std::coroutine_handle<> h;

            auto operator<=>(const timer_task& other) const {
                return time <=> other.time;
            }
        };

        // struct user_data {
        //
        // };

        static void set_thread_live_reactor(reactor* reactor);

        static void clear_thread_live_reactor();

        void update_timer_tasks();

        std::optional<std::chrono::steady_clock::time_point> get_first_time_expire();

        collections::SortedLinkedList<timer_task> time_tasks;
        collections::Ring<std::coroutine_handle<>> ready_queue;
        collections::Ring<std::coroutine_handle<>> sqe_waiters;

        std::int64_t live_uring_tasks = 0;

        io_uring ring;

        // memory::Pool<user_data> data_pool;

        static __kernel_timespec to_ktimespec(std::chrono::steady_clock::time_point tp);

    public:
        /**
         * @brief initializes a new reactor with an ioring buffer size of 128 entries
         * @param size 128 entries seems pretty reasonable by default
         */
        reactor(std::uint32_t size = 128);

        [[nodiscard]] bool empty() const;

        void enqueue(std::coroutine_handle<> handle);

        void add_sleep_task(std::chrono::steady_clock::time_point resume, std::coroutine_handle<> h);

        void run();

        static reactor* get_thread_reactor();

        io_uring_sqe *get_sqe(uring_promise& h);

        void submit();

        void process_cqe(io_uring_cqe* cqe);

        void enqueue_sqe_waiter(std::coroutine_handle<> handle);

        io_uring* get_uring_ptr();
    };
}
