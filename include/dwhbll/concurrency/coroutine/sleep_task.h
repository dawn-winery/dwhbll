#pragma once

#include <chrono>
#include <coroutine>

namespace dwhbll::concurrency::coroutine {
    class sleep_task {
        std::chrono::steady_clock::time_point _finish;

    public:
        [[nodiscard]] explicit sleep_task(const std::chrono::steady_clock::time_point &finish)
            : _finish(finish) {
        }

        [[nodiscard]] bool await_ready() const noexcept;

        void await_suspend(std::coroutine_handle<> h) const noexcept;

        void await_resume() noexcept;
    };

    template <typename Dur>
    sleep_task sleep_for(Dur d) {
        return sleep_task{std::chrono::steady_clock::now() + d};
    }
}
