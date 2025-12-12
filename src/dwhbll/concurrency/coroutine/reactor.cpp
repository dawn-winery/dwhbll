#include <dwhbll/concurrency/coroutine/reactor.h>

#include <dwhbll/exceptions/rt_exception_base.h>
#include <dwhbll/sanify/deferred.h>
#include <thread>

namespace dwhbll::concurrency::coroutine {
    namespace detail {
        thread_local reactor* live_reactor;

        void reactor_enqueue(std::coroutine_handle<> h) {
            reactor::get_thread_reactor()->enqueue(h);
        }
    }

    void reactor::set_thread_live_reactor(reactor *reactor) {
        detail::live_reactor = reactor;
    }

    void reactor::clear_thread_live_reactor() {
        detail::live_reactor = nullptr;
    }

    void reactor::update_timer_tasks() {
        auto now = std::lower_bound(time_tasks.begin(), time_tasks.end(), timer_task{std::chrono::steady_clock::now()});

        for (auto b = time_tasks.begin(); b != now; ++b)
            ready_queue.push_back(b->h);

        time_tasks.erase(time_tasks.begin(), now);
    }

    std::optional<std::chrono::steady_clock::time_point> reactor::get_first_time_expire() {
        if (time_tasks.empty())
            return std::nullopt;
        return time_tasks.front().time;
    }

    bool reactor::empty() const {
        return ready_queue.empty() && time_tasks.empty();
    }

    void reactor::enqueue(std::coroutine_handle<> handle) {
        ready_queue.push_back(handle);
    }

    void reactor::add_sleep_task(std::chrono::steady_clock::time_point resume, std::coroutine_handle<> h) {
        if (resume < std::chrono::steady_clock::now())
            ready_queue.push_back(h);
        else
            time_tasks.insert({resume, h});
    }

    void reactor::run() {
        set_thread_live_reactor(this);
        sanify::deferred task([] {
            clear_thread_live_reactor();
        });

        while (!empty()) {
            auto first_expire = get_first_time_expire();
            if (first_expire.has_value())
                std::this_thread::sleep_until(first_expire.value());
            update_timer_tasks();
            while (!ready_queue.empty()) {
                auto front = ready_queue.front();
                ready_queue.pop_front();
                front.resume();
            }
        }

    }

    reactor * reactor::get_thread_reactor() {
        if (!detail::live_reactor)
            throw exceptions::rt_exception_base("There is no currently running reactor on this thread!");
        return detail::live_reactor;
    }
}
