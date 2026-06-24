#pragma once

#include <functional>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/stl_ext/common_helpers.h>

// TODO: Spend 5 afternoons reading the C++ spec and figuring out how to optimize this garbage to be more user friendly.
// TODO: Function template to accept universal
namespace dwhbll::stl_ext {
    /**
     * @brief rust but in c++ I mean what?
     * @tparam T Value of Ok variant
     * @tparam E Value of Err Variant
     */
    template <typename T, typename E>
    requires (!std::same_as<T, void> && !std::same_as<E, void>)
    class Result {
    public:
        enum TYPE {
            Ok,
            Err,
        } type;

        struct DUMMY_TYPE_NEVER{};
        union DATA {
            DUMMY_TYPE_NEVER NEVER{};
            T OK_VALUE;
            E ERR_VALUE;

            ~DATA() {}
        } data;

        Result(const __detail::result_ok_helper<T>&& ok_val) {
            type = Ok;
            new (&data.OK_VALUE) T(std::move(ok_val.value));
        }

        Result(const __detail::result_err_helper<E>&& err_val) {
            type = Err;
            new (&data.ERR_VALUE) E(std::move(err_val.value));
        }

        Result(const T&& ok_val)
            requires (!std::is_same_v<std::decay_t<E>, std::decay_t<T>>) :
            Result(__detail::result_ok_helper<T>(std::move(ok_val))) {}

        Result(const E&& err_val)
            requires (!std::is_same_v<std::decay_t<E>, std::decay_t<T>>) :
            Result(__detail::result_err_helper<E>(std::move(err_val))) {}

        Result(const Result &other) {
            type = other.type;
            switch (other.type) {
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
            type = other.type;
            switch (other.type) {
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
            type = other.type;
            switch (other.type) {
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
            switch (type) {
            case Ok:
                data.OK_VALUE.~T();
                break;
            case Err:
                data.ERR_VALUE.~E();
                break;
            }
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

        // No rust Option :xdd:, todo: ok(), err()
        std::optional<T> ok() {
            if (type == Err)
                return std::nullopt;
            return data.OK_VALUE;
        }

        std::optional<E> err() {
            if (type == Ok)
                return std::nullopt;
            return data.ERR_VALUE;
        }

        template <typename U>
        Result<U, E> map(std::function<U(T)> func) const noexcept {
            if (type == Err)
                return Result<U, E>(Err(data.ERR_VALUE));
            return Result<U, E>(func(data.OK_VALUE));
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
                return Result<T, U>(Ok(data.OK_VALUE));
            return Result<T, U>(func(data.ERR_VALUE));
        }

        // TODO: inspect (C++ std::visit)

        decltype(auto) expect(this auto&& self, const std::string& msg) {
            // TODO: Check for std::format specialization for E type
            if (self.type == Err)
                debug::panic("{}: called `Result::expect()` on an `Err` value", msg);
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
                debug::panic("{}: called `Result::expect_err()` on an `Ok` value", msg);
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        decltype(auto) unwrap_err(this auto&& self) {
            // TODO: Check for std::format specialization for E type
            if (self.type == Ok)
                debug::panic("called `Result::unwrap_err()` on an `Ok` value");
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        template <typename U>
        Result<U, E> and_(this auto&& self, const Result<U, E>&& res) {
            if (self.type == Err)
                return Result<U, E>(Err(std::forward<decltype(self)>(self).data.ERR_VALUE));
            return res;
        }

        template <typename U>
        Result<U, E> and_then(this auto&& self, const std::function<Result<U, E>(T)>& op) {
            if (self.type == Ok)
                return op(std::forward<decltype(self)>(self).data.OK_VALUE);
            return Result<U, E>(Err(std::forward<decltype(self)>(self).data.ERR_VALUE));
        }

        template <typename F>
        Result<T, F> or_(this auto&& self, const Result<T, F>&& res) {
            if (self.type == Err)
                return res;
            return Result<T, F>(std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        template <typename F>
        Result<T, F> or_else(this auto&& self, const std::function<Result<T, F>(E)>& op) {
            if (self.type == Err)
                return op(std::forward<decltype(self)>(self).data.ERR_VALUE);
            return Result<T, F>(Ok(std::forward<decltype(self)>(self).data.OK_VALUE));
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
