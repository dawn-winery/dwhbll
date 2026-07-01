#pragma once

#include <chrono>
#include <coroutine>
#include <future>
#include <unordered_set>

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
        struct job;
        struct user_data;

        struct timer_task {
            std::chrono::steady_clock::time_point time;
            user_data* data;

            auto operator<=>(const timer_task& other) const {
                return time <=> other.time;
            }
        };

        /**
         * @brief structure stored in an iouring completion request.
         */
        struct user_data {
            job* parent = nullptr;
            void* promise = nullptr;
            std::coroutine_handle<> handle;
        };

        /**
         * @brief Represents one coroutine job, completions are associated with
         * a job to facilitate cancellation
         */
        struct job {
            job* parent = nullptr;
            std::unordered_set<job*> children{};
            std::unordered_set<user_data*> completions{};
        };

        static void set_thread_live_reactor(reactor* reactor);

        static void clear_thread_live_reactor();

        void update_timer_tasks();

        std::optional<std::chrono::steady_clock::time_point> get_first_time_expire();

        collections::SortedLinkedList<timer_task> time_tasks;
        collections::Ring<user_data*> ready_queue;
        collections::Ring<user_data*> sqe_waiters;

        std::int64_t inflight_jobs = 0; ///< Number of jobs in flight (tasks)
        std::int64_t inflight_completions = 0; ///< Number of completions in flight (waiting on kernel)
        // std::int64_t inflight_waits = 0; ///< Number of waits in flight (waiting on some reactor event)

        io_uring ring;

        memory::Pool<user_data> data_pool;
        memory::Pool<job> job_pool;

        job* current_job = nullptr;

        static __kernel_timespec to_ktimespec(std::chrono::steady_clock::time_point tp);

        /**
         * @param data User Data ptr
         * @note Consumes user_data!
         */
        void resume_stall_check(user_data *data, std::coroutine_handle<> h);

        user_data* user_data_lifetime_begin();

        void user_data_lifetime_end(user_data *data);

        job* job_lifetime_begin();

        void job_lifetime_end(job *job);

    public:
        /**
         * @brief initializes a new reactor with an ioring buffer size of 128 entries
         * @param size 128 entries seems pretty reasonable by default
         */
        reactor(std::uint32_t size = 128);

        ~reactor();

        [[nodiscard]] bool empty() const;

        void enqueue(std::coroutine_handle<> handle);

        void add_sleep_task(std::chrono::steady_clock::time_point resume, std::coroutine_handle<> h);

        void run();

        void spawn(task<> future);

        [[nodiscard]] std::future<void> spawn_with_future(task<> future);

        static reactor* get_thread_reactor();

        io_uring_sqe *get_sqe(uring_promise& h);

        void submit();

        void process_cqe(io_uring_cqe* cqe);

        void enqueue_sqe_waiter(std::coroutine_handle<> handle);

        io_uring* get_uring_ptr();
    };
}
