#pragma once

#include <concepts>
#include <functional>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/stl_ext/common_helpers.h>

// TODO: Spend 5 afternoons reading the C++ spec and figuring out how to optimize this garbage to be more user friendly.
// TODO: Function template to accept universal
namespace dwhbll::stl_ext {
    template <typename T>
    requires (!std::same_as<T, void>)
    class Option;

    /**
     * @brief rust but in c++ I mean what?
     * @tparam T Value of Ok variant
     * @tparam E Value of Err Variant
     */
    template <typename T, typename E>
    requires (!std::same_as<T, void> && !std::same_as<E, void>)
    class Result {
        enum TYPE {
            Invalid,
            Ok,
            Err,
        } type{};

        struct DUMMY_TYPE_NEVER{};
        union DATA {
            DUMMY_TYPE_NEVER NEVER{};
            T OK_VALUE;
            E ERR_VALUE;

            ~DATA() {}
        } data;

        void __destroy_storage() {
            switch (type) {
            case Invalid:
                break;
            case Ok:
                data.OK_VALUE.~T();
                break;
            case Err:
                data.ERR_VALUE.~E();
                break;
            }
        }

    public:
        template <typename TV>
        requires (std::is_convertible_v<TV, T>)
        Result(const __detail::result_ok_helper<TV>&& ok_val) {
            type = Ok;
            new (&data.OK_VALUE) T(std::move(ok_val.value));
        }

        template <typename EV>
        requires (std::is_convertible_v<EV, E>)
        Result(const __detail::result_err_helper<EV>&& err_val) {
            type = Err;
            new (&data.ERR_VALUE) E(std::move(err_val.value));
        }

        template <typename TV>
        requires (!std::is_same_v<std::decay_t<E>, std::decay_t<T>> && std::is_convertible_v<TV, T>)
        Result(const TV&& ok_val) :
            Result(__detail::result_ok_helper<T>(std::move(ok_val))) {}

        template <typename EV>
        requires (!std::is_same_v<std::decay_t<E>, std::decay_t<T>> && std::is_convertible_v<EV, E>)
        Result(const EV&& err_val) :
            Result(__detail::result_err_helper<E>(std::move(err_val))) {}

        Result(const Result &other) {
            type = other.type;
            switch (other.type) {
            case Invalid:
                debug::panic();
            case Ok:
                new (&data.OK_VALUE) T(other.data.OK_VALUE);
                break;
            case Err:
                new (&data.ERR_VALUE) E(other.data.ERR_VALUE);
                break;
            }
        }

        Result(Result &&other) noexcept {
            type = other.type;
            switch (other.type) {
            case Invalid:
                debug::panic();
            case Ok:
                new (&data.OK_VALUE) T(std::move(other.data.OK_VALUE));
                break;
            case Err:
                new (&data.ERR_VALUE) E(std::move(other.data.ERR_VALUE));
                break;
            }
        }

        Result & operator=(const Result &other) {
            if (this == &other)
                return *this;
            __destroy_storage();
            type = other.type;
            switch (other.type) {
            case Invalid:
                debug::panic();
            case Ok:
                new (&data.OK_VALUE) T(other.data.OK_VALUE);
                break;
            case Err:
                new (&data.ERR_VALUE) E(other.data.ERR_VALUE);
                break;
            }
            return *this;
        }

        Result & operator=(Result &&other) noexcept {
            if (this == &other)
                return *this;
            __destroy_storage();
            type = other.type;
            switch (other.type) {
            case Invalid:
                debug::panic();
            case Ok:
                new (&data.OK_VALUE) T(std::move(other.data.OK_VALUE));
                break;
            case Err:
                new (&data.ERR_VALUE) E(std::move(other.data.ERR_VALUE));
                break;
            }
            return *this;
        }

        ~Result() {
            __destroy_storage();
        }

        [[nodiscard]] constexpr bool is_ok() const noexcept {
            return type == Ok;
        }

        [[nodiscard]] constexpr bool is_ok_and(std::function<bool(T)> func) const noexcept {
            return type == Ok && func(data.OK_VALUE);
        }

        [[nodiscard]] constexpr bool is_err() const noexcept {
            return type == Err;
        }

        [[nodiscard]] constexpr bool is_err_and(std::function<bool(E)> func) const noexcept {
            return type == Err && func(data.ERR_VALUE);
        }

        Option<T> ok(this auto&& self)  {
            if (self.type == Err)
                return Option<T>();
            return Some(std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        Option<E> err(this auto&& self) {
            if (self.type == Ok)
                return Option<E>();
            return Some(std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        template <typename F, typename U = std::invoke_result_t<F, T>>
        auto map(F&& func) const noexcept -> Result<U, E> {
            if (type == Err)
                return Result<U, E>(__detail::result_err_helper<E>(data.ERR_VALUE));
            return Result<U, E>(__detail::result_ok_helper<U>(func(data.OK_VALUE)));
        }

        template <typename U>
        U map_or(const U&& def, std::function<U(T)> func) const noexcept {
            if (type == Err)
                return std::forward<U>(def);
            return func(data.OK_VALUE);
        }

        template <typename U>
        U map_or(std::function<U(E)> def, std::function<U(T)> func) const noexcept {
            if (type == Err)
                return def(data.ERR_VALUE);
            return func(data.OK_VALUE);
        }

        template <typename U>
        Result<T, U> map_err(std::function<U(E)> func) const noexcept {
            if (type == Ok)
                return Result<T, U>(__detail::result_ok_helper<T>(data.OK_VALUE));
            return Result<T, U>(func(data.ERR_VALUE));
        }

        // TODO: inspect (C++ std::visit)

        decltype(auto) expect(this auto&& self, const std::string& msg) {
            // TODO: Check for std::format specialization for E type
            if (self.type == Err)
                debug::panic("{}", msg); // {}: in future
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        decltype(auto) unwrap(this auto&& self) {
            // TODO: Check for std::format specialization for E type
            if (self.type == Err)
                debug::panic("called `Result::unwrap()` on an `Err` value");
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        decltype(auto) unwrap_or_default(this auto&& self)
            requires std::is_default_constructible_v<T> {
            if (self.type == Err)
                return T{};
            return (std::forward<decltype(self)>(self).OK_VALUE);
        }

        decltype(auto) expect_err(this auto&& self, const std::string& msg) {
            // TODO: Check for std::format specialization for E type
            if (self.type == Ok)
                debug::panic("{}", msg); // {}: in future
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        decltype(auto) unwrap_err(this auto&& self) {
            // TODO: Check for std::format specialization for E type
            if (self.type == Ok)
                debug::panic("called `Result::unwrap_err()` on an `Ok` value");
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        template <typename U>
        Result<U, E> and_(this auto&& self, const Result<U, E>& res) {
            if (self.type == Err)
                return Result<U, E>(__detail::result_err_helper<E>(std::forward<decltype(self)>(self).data.ERR_VALUE));
            return res;
        }

        template <typename U>
        Result<U, E> and_then(this auto&& self, const std::function<Result<U, E>(T)>& op) {
            if (self.type == Ok)
                return op(std::forward<decltype(self)>(self).data.OK_VALUE);
            return Result<U, E>(__detail::result_err_helper<E>(std::forward<decltype(self)>(self).data.ERR_VALUE));
        }

        template <typename F>
        Result<T, F> or_(this auto&& self, const Result<T, F>& res) {
            if (self.type == Err)
                return res;
            return Result<T, F>(std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        template <typename F>
        Result<T, F> or_else(this auto&& self, const std::function<Result<T, F>(E)>& op) {
            if (self.type == Err)
                return op(std::forward<decltype(self)>(self).data.ERR_VALUE);
            return Result<T, F>(__detail::result_ok_helper<T>(std::forward<decltype(self)>(self).data.OK_VALUE));
        }

        decltype(auto) unwrap_or(this auto&& self, T&& def) {
            if (self.type == Err)
                return def;
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        decltype(auto) unwrap_or_else(this auto&& self, const std::function<T(E)>& def) {
            if (self.type == Err)
                return def(std::forward<decltype(self)>(self).data.ERR_VALUE);
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        decltype(auto) unwrap_unchecked(this auto&& self) {
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        decltype(auto) unwrap_err_unchecked(this auto&& self) {
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }
    };
}
