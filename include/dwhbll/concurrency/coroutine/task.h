#pragma once

#include <coroutine>
#include <optional>
#include <dwhbll/exceptions/concurrency_exception.h>

namespace dwhbll::concurrency::coroutine {
    namespace detail {
        void reactor_enqueue(std::coroutine_handle<> h);
    }

    template <typename T = void>
    class task {
    public:
        struct promise {
            std::optional<T> value;
            std::coroutine_handle<> continuation;

            task get_return_object();

            std::suspend_always initial_suspend() noexcept;

            auto final_suspend() noexcept;

            void return_value(T v) noexcept;

            void unhandled_exception() noexcept;
        };

        using handle_t = std::coroutine_handle<promise>;
        using promise_type = promise;

    private:
        handle_t coroutine;

    public:
        explicit task(handle_t h) : coroutine(h) {}

        task(const task&) = delete;

        task(task&& other) noexcept {
            if (coroutine)
                coroutine.destroy();
            coroutine = other.coroutine;
            other.coroutine = {};
        }

        ~task() {
            if (coroutine)
                coroutine.destroy();
        }

        task & operator=(const task &other) = delete;

        task & operator=(task &&other) noexcept {
            if (this == &other)
                return *this;
            if (coroutine)
                coroutine.destroy();
            coroutine = std::move(other.coroutine);
            other.coroutine = {};
            return *this;
        }

        struct continuation_awaiter {
            handle_t h;

            [[nodiscard]] bool await_ready() const noexcept {
                return !h || h.done();
            }

            auto await_suspend(std::coroutine_handle<> parent) const noexcept {
                h.promise().continuation = parent;

                detail::reactor_enqueue(h);
            }

            T await_resume() {
                T value = std::move(h.promise().value.value());

                if (h)
                    h.destroy();

                return std::move(value);
            }
        };

        struct finalize_awaiter {
            bool await_ready() noexcept {
                return false;
            }
            void await_suspend(handle_t h) noexcept {
                if (h.promise().continuation)
                    detail::reactor_enqueue(h.promise().continuation);
            }
            void await_resume() noexcept {}
        };

        auto operator co_await() {
            auto coro = std::move(coroutine);
            coroutine = {};
            return continuation_awaiter{coro};
        }

        handle_t get_handle() const noexcept {
            return coroutine;
        }
    };

    template<typename T>
    task<T> task<T>::promise::get_return_object() {
        return task {handle_t::from_promise(*this)};
    }

    template<typename T>
    std::suspend_always task<T>::promise::initial_suspend() noexcept {
        return {};
    }

    template<typename T>
    auto task<T>::promise::final_suspend() noexcept {
        return finalize_awaiter{};
    }

    template<typename T>
    void task<T>::promise::return_value(T v) noexcept {
        value = std::move(v);
    }

    template<typename T>
    void task<T>::promise::unhandled_exception() noexcept {
        // TODO!
        std::terminate();
    }

    template<>
    class task<void> {
    public:
        struct promise {
            std::coroutine_handle<> continuation;

            task get_return_object();

            std::suspend_always initial_suspend() noexcept;

            auto final_suspend() noexcept;

            void return_void() noexcept;

            void unhandled_exception() noexcept;
        };

        using handle_t = std::coroutine_handle<promise>;
        using promise_type = promise;

    private:
        handle_t coroutine;

    public:
        explicit task(handle_t h) : coroutine(h) {}

        task(const task&) = delete;

        task(task&& other) noexcept {
            if (coroutine)
                coroutine.destroy();
            coroutine = other.coroutine;
            other.coroutine = {};
        }

        ~task() {
            if (coroutine)
                coroutine.destroy();
        }

        task & operator=(const task &other) = delete;

        task & operator=(task &&other) noexcept {
            if (this == &other)
                return *this;
            if (coroutine)
                coroutine.destroy();
            coroutine = other.coroutine;
            other.coroutine = {};
            return *this;
        }

        struct continuation_awaiter {
            handle_t h;

            [[nodiscard]] bool await_ready() const noexcept {
                return !h || h.done();
            }

            auto await_suspend(std::coroutine_handle<> parent) const noexcept {
                h.promise().continuation = parent;

                detail::reactor_enqueue(h);
            }

            void await_resume() {
                h.destroy();
            }
        };

        struct finalize_awaiter {
            bool reactor_owned;

            bool await_ready() noexcept {
                return false;
            }

            void await_suspend(handle_t h) noexcept {
                if (h.promise().continuation)
                    detail::reactor_enqueue(h.promise().continuation);
            }

            void await_resume() noexcept {}
        };

        auto operator co_await() {
            auto coro = coroutine;
            coroutine = {};
            return continuation_awaiter{coro};
        }

        [[nodiscard]] handle_t get_handle() const noexcept {
            return coroutine;
        }
    };

    inline task<> task<>::promise::get_return_object() {
        return task {handle_t::from_promise(*this)};
    }

    inline std::suspend_always task<>::promise::initial_suspend() noexcept {
        return {};
    }

    inline auto task<>::promise::final_suspend() noexcept {
        return finalize_awaiter{};
    }

    inline void task<void>::promise::return_void() noexcept {}

    inline void task<>::promise::unhandled_exception() noexcept {
        // TODO!
        std::terminate();
    }
}
