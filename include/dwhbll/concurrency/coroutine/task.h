#pragma once

#include <coroutine>
#include <optional>
#include <expected>

#include <dwhbll/concurrency/coroutine/cancellable_base.h>
#include <dwhbll/debug/debug.h>
#include <dwhbll/exceptions/concurrency_exception.h>
#include <dwhbll/stl_ext/result.h>

namespace dwhbll::concurrency::coroutine {
    template <typename T>
    class task;

    namespace detail {
        void reactor_enqueue(cancellable_base* cancellable, std::coroutine_handle<> h);

        template <typename T>
        struct promise;

        template <typename T = void>
        struct final_awaiter : cancellable_base {
            using handle_t = std::coroutine_handle<promise<T>>;

            constexpr bool await_ready() noexcept {
                return false;
            }

            constexpr void await_suspend(handle_t h) noexcept {
                if (h.promise().continuation)
                    detail::reactor_enqueue(this, h.promise().continuation);
            }

            constexpr void await_resume() noexcept {
                cancellable_base::await_resume();
            }
        };

        template <typename T = void>
        struct promise {
            using handle_t = std::coroutine_handle<promise>;

            std::optional<std::expected<T, std::exception_ptr>> data{};
            std::coroutine_handle<> continuation{};

            constexpr task<T> get_return_object() {
                return task {handle_t::from_promise(*this)};
            }

            constexpr std::suspend_always initial_suspend() noexcept {
                return {};
            }

            constexpr detail::final_awaiter<T> final_suspend() noexcept {
                return {};
            }

            constexpr void unhandled_exception() noexcept {
                data = std::unexpected(std::current_exception());
            }

            constexpr void return_value(T v) noexcept {
                data = std::move(v);
            }
        };

        template<>
        struct promise<void> {
            using handle_t = std::coroutine_handle<promise>;

            std::optional<std::expected<stl_ext::UNIT, std::exception_ptr>> data;
            std::coroutine_handle<> continuation;

            constexpr task<void> get_return_object();

            constexpr std::suspend_always initial_suspend() noexcept {
                return {};
            }

            constexpr final_awaiter<> final_suspend() noexcept {
                return {};
            }

            constexpr void return_void() noexcept {
                data = stl_ext::UNIT{};
            }

            constexpr void unhandled_exception() noexcept {
                data = std::unexpected(std::current_exception());
            }
        };

        template <typename T = void>
        struct base_continuation_awaiter : cancellable_base {
            using handle_t = std::coroutine_handle<promise<T>>;

            handle_t h;

            base_continuation_awaiter(handle_t h) : cancellable_base(), h(h) {}

            [[nodiscard]] constexpr bool await_ready() const noexcept {
                return !h || h.done();
            }

            auto await_suspend(std::coroutine_handle<> parent) noexcept {
                h.promise().continuation = parent;

                detail::reactor_enqueue(this, h);
            }
        };

        template <typename T = void>
        struct continuation_awaiter : base_continuation_awaiter<T> {
            using base_continuation_awaiter<T>::base_continuation_awaiter;

            T await_resume() {
                if (base_continuation_awaiter<T>::is_cancelled()) {
                    base_continuation_awaiter<T>::h.destroy();
                    base_continuation_awaiter<T>::await_resume();
                    debug::unreachable();
                }

                auto& promise_data = base_continuation_awaiter<T>::h.promise().data;

                if (!promise_data.has_value())
                    debug::unreachable();

                if (!promise_data.value().has_value()) {
                    std::exception_ptr eptr = promise_data.value().error();

                    if (base_continuation_awaiter<T>::h)
                        base_continuation_awaiter<T>::h.destroy();

                    std::rethrow_exception(eptr);
                }

                T value = std::move(promise_data.value().value());

                if (base_continuation_awaiter<T>::h)
                    base_continuation_awaiter<T>::h.destroy();

                return std::move(value);
            }
        };

        template <>
        struct continuation_awaiter<void> : public base_continuation_awaiter<void> {
            using base_continuation_awaiter<void>::base_continuation_awaiter;

            void await_resume() {
                if (is_cancelled()) {
                    h.destroy();
                    base_continuation_awaiter<void>::await_resume();
                    debug::unreachable();
                }

                auto& promise_data = h.promise().data;

                if (!promise_data.has_value())
                    debug::unreachable();

                if (!promise_data.value().has_value()) {
                    std::exception_ptr eptr = promise_data.value().error();

                    if (h)
                        h.destroy();

                    std::rethrow_exception(eptr);
                }

                if (h)
                    h.destroy();
            }
        };
    }

    template <typename T = void>
    class task {
    public:
        using handle_t = std::coroutine_handle<detail::promise<T>>;
        using promise_type = detail::promise<T>;

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

        auto operator co_await() {
            auto coro = std::move(coroutine);
            coroutine = {};
            return detail::continuation_awaiter<T>{coro};
        }

        handle_t get_handle() const noexcept {
            return coroutine;
        }
    };

    template<>
    class task<void> {
    public:
        using handle_t = std::coroutine_handle<detail::promise<>>;
        using promise_type = detail::promise<>;

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

        auto operator co_await() {
            auto coro = coroutine;
            coroutine = {};
            return detail::continuation_awaiter<void>{coro};
        }

        [[nodiscard]] handle_t get_handle() const noexcept {
            return coroutine;
        }
    };

    namespace detail {
        constexpr task<> promise<void>::get_return_object() {
            return task {handle_t::from_promise(*this)};
        }
    }
}
