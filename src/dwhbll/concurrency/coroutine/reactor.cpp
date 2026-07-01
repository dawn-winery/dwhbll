#include <dwhbll/concurrency/coroutine/reactor.h>

#include <dwhbll/exceptions/rt_exception_base.h>
#include <thread>
#include <liburing.h>
#include <dwhbll/concurrency/coroutine/detached_task.h>
#include <dwhbll/concurrency/coroutine/uring_promise.h>
#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/stl_ext/utilities.h>

namespace dwhbll::concurrency::coroutine {
    namespace detail {
        thread_local reactor* live_reactor;

        void reactor_enqueue(std::coroutine_handle<> h) {
            reactor::get_thread_reactor()->enqueue(h);
        }
    }

    void reactor::set_thread_live_reactor(reactor *reactor) {
        if (detail::live_reactor)
            debug::panic("there is already a live reactor on this thread!");
        detail::live_reactor = reactor;
    }

    void reactor::clear_thread_live_reactor() {
        detail::live_reactor = nullptr;
    }

    void reactor::update_timer_tasks() {
        auto now = std::lower_bound(time_tasks.begin(), time_tasks.end(), timer_task{std::chrono::steady_clock::now()});

        for (auto b = time_tasks.begin(); b != now; ++b)
            ready_queue.push_back(b->data);

        time_tasks.erase(time_tasks.begin(), now);
    }

    std::optional<std::chrono::steady_clock::time_point> reactor::get_first_time_expire() {
        if (time_tasks.empty())
            return std::nullopt;
        return time_tasks.front().time;
    }

    __kernel_timespec reactor::to_ktimespec(std::chrono::steady_clock::time_point tp) {
        auto diff = tp - std::chrono::steady_clock::now();

        auto secs = std::chrono::duration_cast<std::chrono::seconds>(diff);
        diff -= secs;

        return __kernel_timespec{secs.count(), diff.count()};
    }

    void reactor::resume_stall_check(user_data *data, std::coroutine_handle<> h) {
        auto begin = std::chrono::steady_clock::now();

        if (!data->parent)
            debug::panic("Continuation has no associated job!");

        {
            auto _ = stl_ext::store_temporary(current_job, data->parent);
            h.resume();
        }

        auto total_time = std::chrono::steady_clock::now() - begin;

        if (total_time > std::chrono::milliseconds(5))
            console::warn("reactor stall detected! reactor stalled for {}", total_time);

        user_data_lifetime_end(data);
    }

    reactor::user_data * reactor::user_data_lifetime_begin() {
        auto* obj = data_pool.acquire(current_job, nullptr).disown();

        current_job->completions.insert(obj);

        return obj;
    }

    void reactor::user_data_lifetime_end(user_data *data) {
        auto* parent_job = data->parent;

        // get rid of parent's completion wait.
        parent_job->completions.erase(data);

        if (parent_job->completions.empty() && parent_job->children.empty())
            job_lifetime_end(parent_job); // job lifetime is over if it has nothing left.

        data_pool.offer(data);
    }

    reactor::job * reactor::job_lifetime_begin() {
        // start a job who's parent is the current job.
        auto* job = job_pool.acquire(current_job).disown();

        if (current_job) // job might be root.
            current_job->children.insert(job);

        inflight_jobs++;

        return job;
    }

    void reactor::job_lifetime_end(job *job) {
        auto* parent_job = job->parent;

        if (parent_job) {
            // have parent waiting on us
            parent_job->children.erase(job);

            if (parent_job->completions.empty() && parent_job->children.empty())
                job_lifetime_end(parent_job); // job lifetime is over
        }

        inflight_jobs--;

        job_pool.offer(job);
    }

    reactor::reactor(std::uint32_t size) : ring() {
        auto r = io_uring_queue_init(size, &ring, IORING_SETUP_SQPOLL);
        if (r < 0)
            debug::panic("failed to setup uring queue! ({})", strerror(errno));

        set_thread_live_reactor(this);
    }

    reactor::~reactor() {
        io_uring_queue_exit(&ring);

        clear_thread_live_reactor();
    }

    bool reactor::empty() const {
        return ready_queue.empty() && time_tasks.empty() && sqe_waiters.empty() && inflight_completions == 0;
    }

    void reactor::enqueue(std::coroutine_handle<> handle) {
        auto* data = user_data_lifetime_begin();
        data->handle = handle;
        ready_queue.push_back(data);
    }

    void reactor::add_sleep_task(std::chrono::steady_clock::time_point resume, std::coroutine_handle<> h) {
        auto* data = user_data_lifetime_begin();
        data->handle = h;

        if (resume < std::chrono::steady_clock::now())
            ready_queue.push_back(data);
        else
            time_tasks.insert({resume, data});
    }

    void reactor::run() {
        while (!empty()) {
            auto first_expire = get_first_time_expire();

            io_uring_cqe *cqe;

            // only if ready_queue is not empty
            if (ready_queue.empty()) {
                int available;
                if (first_expire.has_value()) {
                    __kernel_timespec ts = to_ktimespec(first_expire.value());
                    available = io_uring_wait_cqe_timeout(&ring, &cqe, &ts);
                } else {
                    available = io_uring_wait_cqe(&ring, &cqe);
                }

                if (available == 0)
                    process_cqe(cqe);
            }

            // process all the remaining CQEs (if there's any at all)
            while (io_uring_peek_cqe(&ring, &cqe) == 0)
                process_cqe(cqe);

            // it's probably a good idea to drain all of this before we keep going
            update_timer_tasks();
            while (!ready_queue.empty()) {
                auto front = ready_queue.front();
                ready_queue.pop_front();
                resume_stall_check(front, front->handle);
            }

            while (!sqe_waiters.empty() &&
                io_uring_sq_space_left(&ring) > 0) {

                auto h = sqe_waiters.front();
                sqe_waiters.pop_front();
                resume_stall_check(h, h->handle);
           }
        }
    }

    void reactor::spawn(task<> future) {
        auto f = [fut = std::move(future)]() mutable -> DetachedTask {
            try {
                co_await fut;
            } catch (const exceptions::rt_exception_base& e) {
                exceptions::rt_exception_base::traceback_terminate_handler();
                debug::panic("uncaught exception unwound through future");
            } catch (const std::runtime_error& e) {
                exceptions::rt_exception_base::traceback_terminate_handler();
                debug::panic("uncaught exception unwound through future.");
            } catch (...) {
                auto eptr = std::current_exception();
                auto tname = eptr.__cxa_exception_type()->name();
                debug::panic("unknown uncaught exception (type: {})", tname);
            }
        };

        auto* job = job_lifetime_begin();
        auto _ = stl_ext::store_temporary(current_job, job);
        f();
    }

    std::future<void> reactor::spawn_with_future(task<> future) {
        std::promise<void> promise;
        auto fut = promise.get_future();
        auto f = [fut = std::move(future), promise=std::move(promise)]() mutable -> DetachedTask {
            try {
                co_await fut;
                promise.set_value();
            } catch (...) {
                auto eptr = std::current_exception();
                promise.set_exception(eptr);
            }
        };

        auto* job = job_lifetime_begin();
        auto _ = stl_ext::store_temporary(current_job, job);
        f();

        return fut;
    }

    reactor * reactor::get_thread_reactor() {
        if (!detail::live_reactor)
            throw exceptions::rt_exception_base("There is no currently running reactor on this thread!");
        return detail::live_reactor;
    }

    io_uring_sqe *reactor::get_sqe(uring_promise& h) {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);

        if (!sqe)
            return nullptr;

        auto* data = user_data_lifetime_begin();
        data->promise = &h;

        io_uring_sqe_set_data(sqe, data);

        inflight_completions++;

        return sqe;
    }

    void reactor::submit() {
        io_uring_submit(&ring);
    }

    void reactor::process_cqe(io_uring_cqe *cqe) {
        inflight_completions--;

        auto* job_info = static_cast<user_data *>(io_uring_cqe_get_data(cqe));

        auto* promise = static_cast<uring_promise*>(job_info->promise);

        promise->cqe = cqe;

        resume_stall_check(job_info, promise->waiter); // resume the coroutine

        io_uring_cqe_seen(&ring, cqe);
    }

    void reactor::enqueue_sqe_waiter(std::coroutine_handle<> handle) {
        auto* data = user_data_lifetime_begin();
        data->handle = handle;
        sqe_waiters.push_back(data);
    }

    io_uring * reactor::get_uring_ptr() {
        return &ring;
    }
}
