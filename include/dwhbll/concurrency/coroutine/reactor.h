#pragma once

#include <chrono>
#include <coroutine>
#include <dwhbll/collections/ring.h>
#include <dwhbll/collections/sorted_linked_list.h>

namespace dwhbll::concurrency::coroutine {
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

        static void set_thread_live_reactor(reactor* reactor);

        static void clear_thread_live_reactor();

        void update_timer_tasks();

        std::optional<std::chrono::steady_clock::time_point> get_first_time_expire();

        collections::SortedLinkedList<timer_task> time_tasks;
        collections::Ring<std::coroutine_handle<>> ready_queue;

    public:
        [[nodiscard]] bool empty() const;

        void enqueue(std::coroutine_handle<> handle);

        void add_sleep_task(std::chrono::steady_clock::time_point resume, std::coroutine_handle<> h);

        void run();

        static reactor* get_thread_reactor();
    };
}
