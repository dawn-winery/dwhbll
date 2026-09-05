#pragma once

#include <concepts>
#include <functional>
#include <string_view>
#include <utility>

#include <dwhbll/debug/debug.h>
#include <dwhbll/stl_ext/common_helpers.h>

// TODO: Spend 5 afternoons reading the C++ spec and figuring out how to optimize this garbage to be more user friendly.
namespace dwhbll::stl_ext {
    template <typename T>
    requires (!std::same_as<T, void>)
    class Option;

    template <typename T, typename E>
    requires (!std::same_as<T, void> && !std::same_as<E, void>)
    class Result;

    namespace __detail {
        template<typename T>
        struct result_like_traits {
            static constexpr bool value = false;
        };

        template<typename T, typename E>
        struct result_like_traits<Result<T, E>> {
            static constexpr bool value = true;
            using ok_type = T;
            using err_type = E;
        };

        template<typename T>
        struct result_like_traits<result_ok_helper<T>> {
            static constexpr bool value = true;
            using ok_type = T;
            using err_type = void;
        };

        template<typename E>
        struct result_like_traits<result_err_helper<E>> {
            static constexpr bool value = true;
            using ok_type = void;
            using err_type = E;
        };

        template<typename T>
        concept result_like = result_like_traits<std::remove_cvref_t<T>>::value;

        template<typename T>
        using result_like_ok_t = typename result_like_traits<std::remove_cvref_t<T>>::ok_type;

        template<typename T>
        using result_like_err_t = typename result_like_traits<std::remove_cvref_t<T>>::err_type;

        template <typename T, typename E>
        constexpr Result<T, E> to_result(Result<T, E>&& value) {
            return std::move(value);
        }

        template <typename T, typename E>
        constexpr Result<T, E> to_result(const Result<T, E>& value) {
            return value;
        }

        template <typename T, typename E, typename U>
        requires std::constructible_from<T, U>
        constexpr Result<T, E> to_result(result_ok_helper<U>&& value) {
            return Ok(std::move(value.value));
        }

        template <typename T, typename E, typename U>
        requires std::constructible_from<T, const U&>
        constexpr Result<T, E> to_result(const result_ok_helper<U>& value) {
            return Ok(value.value);
        }

        template <typename T, typename E, typename F>
        requires std::constructible_from<E, F>
        constexpr Result<T, E> to_result(result_err_helper<F>&& value) {
            return Err(std::move(value.value));
        }

        template <typename T, typename E, typename F>
        requires std::constructible_from<E, const F&>
        constexpr Result<T, E> to_result(const result_err_helper<F>& value) {
            return Err(value.value);
        }
    }

    /**
     * @brief rust but in c++ I mean what?
     * @tparam T Value of Ok variant
     * @tparam E Value of Err Variant
     */
    template <typename T, typename E>
    requires (!std::same_as<T, void> && !std::same_as<E, void>)
    class Result {
        enum class state : uint8_t {
            invalid,
            ok,
            err,
        };

        struct DUMMY_TYPE_NEVER{};
        union DATA {
            DUMMY_TYPE_NEVER NEVER{};
            T OK_VALUE;
            E ERR_VALUE;

            constexpr ~DATA() {}
        } data;
        state type = state::invalid;

        constexpr void __destroy_storage() {
            switch (type) {
            case state::invalid:
                break;
            case state::ok:
                std::destroy_at(&data.OK_VALUE);
                type = state::invalid;
                break;
            case state::err:
                std::destroy_at(&data.ERR_VALUE);
                type = state::invalid;
                break;
            }
        }

    public:
        template <typename TV>
        requires std::constructible_from<T, TV>
        constexpr Result(__detail::result_ok_helper<TV>&& ok_val)
            noexcept(std::is_nothrow_constructible_v<T, TV>)
        {
            std::construct_at(&data.OK_VALUE, std::move(ok_val.value));
            type = state::ok;
        }

        template <typename TV>
        requires std::constructible_from<T, const TV&>
        constexpr Result(const __detail::result_ok_helper<TV>& ok_val)
            noexcept(std::is_nothrow_constructible_v<T, const TV&>)
        {
            std::construct_at(&data.OK_VALUE, ok_val.value);
            type = state::ok;
        }

        template <typename EV>
        requires std::constructible_from<E, EV>
        constexpr Result(__detail::result_err_helper<EV>&& err_val)
            noexcept(std::is_nothrow_constructible_v<E, EV>)
        {
            std::construct_at(&data.ERR_VALUE, std::move(err_val.value));
            type = state::err;
        }

        template <typename EV>
        requires std::constructible_from<E, const EV&>
        constexpr Result(const __detail::result_err_helper<EV>& err_val)
            noexcept(std::is_nothrow_constructible_v<E, const EV&>)
        {
            std::construct_at(&data.ERR_VALUE, err_val.value);
            type = state::err;
        }

        // If T = E, the Result has to necessarily be built with Ok()/Err()
        template <typename TV>
        requires (!__detail::ok_helper<TV> &&
                  !__detail::err_helper<TV> &&
                  !std::same_as<std::remove_cvref_t<TV>, Result> &&
                  !std::same_as<std::decay_t<T>, std::decay_t<E>> &&
                  (std::same_as<std::remove_cvref_t<TV>, T> ||
                   (std::constructible_from<T, TV&&> && !std::constructible_from<E, TV&&>)))
        constexpr Result(TV&& ok_val)
            noexcept(std::is_nothrow_constructible_v<T, TV&&>)
        {
            std::construct_at(&data.OK_VALUE, std::forward<TV>(ok_val));
            type = state::ok;
        }

        template <typename EV>
        requires (!__detail::ok_helper<EV> &&
                  !__detail::err_helper<EV> &&
                  !std::same_as<std::remove_cvref_t<EV>, Result> &&
                  !std::same_as<std::decay_t<T>, std::decay_t<E>> &&
                  (std::same_as<std::remove_cvref_t<EV>, E> ||
                   (std::constructible_from<E, EV&&> && !std::constructible_from<T, EV&&>)))
        constexpr Result(EV&& err_val)
            noexcept(std::is_nothrow_constructible_v<E, EV&&>)
        {
            std::construct_at(&data.ERR_VALUE, std::forward<EV>(err_val));
            type = state::err;
        }

        constexpr Result(const Result &other)
            requires std::copy_constructible<T> && std::copy_constructible<E>
        {
            if (other.type == state::ok) {
                std::construct_at(&data.OK_VALUE, other.data.OK_VALUE);
                type = state::ok;
            } else if (other.type == state::err) {
                std::construct_at(&data.ERR_VALUE, other.data.ERR_VALUE);
                type = state::err;
            }
        }

        constexpr Result(Result &&other)
            noexcept(std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_move_constructible_v<E>)
            requires std::move_constructible<T> && std::move_constructible<E>
        {
            if (other.type == state::ok) {
                std::construct_at(&data.OK_VALUE, std::move(other.data.OK_VALUE));
                type = state::ok;
            } else if (other.type == state::err) {
                std::construct_at(&data.ERR_VALUE, std::move(other.data.ERR_VALUE));
                type = state::err;
            }
        }

        // TODO: if T or E is not assignable, destroy and reconstruct
        constexpr Result & operator=(const Result &other)
            requires std::copy_constructible<T> &&
                     std::copy_constructible<E> &&
                     std::is_copy_assignable_v<T> &&
                     std::is_copy_assignable_v<E>
        {
            if (this == &other)
                return *this;

            if (type == other.type) {
                if (type == state::ok)
                    data.OK_VALUE = other.data.OK_VALUE;
                else if (type == state::err)
                    data.ERR_VALUE = other.data.ERR_VALUE;
                return *this;
            }

            if (other.type == state::ok) {
                if constexpr (std::is_nothrow_copy_constructible_v<T>) {
                    __destroy_storage();
                    std::construct_at(&data.OK_VALUE, other.data.OK_VALUE);
                    type = state::ok;
                } else {
                    T tmp(other.data.OK_VALUE);
                    __destroy_storage();
                    std::construct_at(&data.OK_VALUE, std::move_if_noexcept(tmp));
                    type = state::ok;
                }
            } else if (other.type == state::err) {
                if constexpr (std::is_nothrow_copy_constructible_v<E>) {
                    __destroy_storage();
                    std::construct_at(&data.ERR_VALUE, other.data.ERR_VALUE);
                    type = state::err;
                } else {
                    E tmp(other.data.ERR_VALUE);
                    __destroy_storage();
                    std::construct_at(&data.ERR_VALUE, std::move_if_noexcept(tmp));
                    type = state::err;
                }
            } else {
                __destroy_storage();
            }

            return *this;
        }

        constexpr Result & operator=(Result &&other)
            noexcept(std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_move_assignable_v<T> &&
                     std::is_nothrow_move_constructible_v<E> &&
                     std::is_nothrow_move_assignable_v<E>)
            requires std::move_constructible<T> &&
                     std::move_constructible<E> &&
                     std::is_move_assignable_v<T> &&
                     std::is_move_assignable_v<E>
        {
            if (this == &other)
                return *this;

            if (type == other.type) {
                if (type == state::ok)
                    data.OK_VALUE = std::move(other.data.OK_VALUE);
                else if (type == state::err)
                    data.ERR_VALUE = std::move(other.data.ERR_VALUE);
                return *this;
            }

            if (other.type == state::ok) {
                if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    __destroy_storage();
                    std::construct_at(&data.OK_VALUE, std::move(other.data.OK_VALUE));
                    type = state::ok;
                } else {
                    T tmp(std::move(other.data.OK_VALUE));
                    __destroy_storage();
                    std::construct_at(&data.OK_VALUE, std::move(tmp));
                    type = state::ok;
                }
            } else if (other.type == state::err) {
                if constexpr (std::is_nothrow_move_constructible_v<E>) {
                    __destroy_storage();
                    std::construct_at(&data.ERR_VALUE, std::move(other.data.ERR_VALUE));
                    type = state::err;
                } else {
                    E tmp(std::move(other.data.ERR_VALUE));
                    __destroy_storage();
                    std::construct_at(&data.ERR_VALUE, std::move(tmp));
                    type = state::err;
                }
            } else {
                __destroy_storage();
            }

            return *this;
        }

        template <typename TV>
        requires std::constructible_from<T, TV>
        constexpr Result & operator=(__detail::result_ok_helper<TV>&& ok_val) {
            if (type == state::ok) {
                if constexpr (std::is_assignable_v<T&, TV>)
                    data.OK_VALUE = std::move(ok_val.value);
                else {
                    std::destroy_at(&data.OK_VALUE);
                    std::construct_at(&data.OK_VALUE, std::move(ok_val.value));
                }
            } else {
                __destroy_storage();
                std::construct_at(&data.OK_VALUE, std::move(ok_val.value));
                type = state::ok;
            }
            return *this;
        }

        template <typename EV>
        requires std::constructible_from<E, EV>
        constexpr Result & operator=(__detail::result_err_helper<EV>&& err_val) {
            if (type == state::err) {
                if constexpr (std::is_assignable_v<E&, EV>)
                    data.ERR_VALUE = std::move(err_val.value);
                else {
                    std::destroy_at(&data.ERR_VALUE);
                    std::construct_at(&data.ERR_VALUE, std::move(err_val.value));
                }
            } else {
                __destroy_storage();
                std::construct_at(&data.ERR_VALUE, std::move(err_val.value));
                type = state::err;
            }
            return *this;
        }

        constexpr ~Result() {
            __destroy_storage();
        }

        [[nodiscard]] constexpr bool is_ok() const noexcept {
            return type == state::ok;
        }

        template <typename F>
        requires std::invocable<F&&, const T&> &&
                 std::convertible_to<std::invoke_result_t<F&&, const T&>, bool>
        [[nodiscard]] constexpr bool is_ok_and(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F&&, const T&>)
        {
            return type == state::ok && std::invoke(std::forward<F>(f), data.OK_VALUE);
        }

        [[nodiscard]] constexpr bool is_err() const noexcept {
            return type == state::err;
        }

        template <typename F>
        requires std::invocable<F&&, const E&> &&
                 std::convertible_to<std::invoke_result_t<F&&, const E&>, bool>
        [[nodiscard]] constexpr bool is_err_and(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F&&, const E&>)
        {
            return type == state::err && std::invoke(std::forward<F>(f), data.ERR_VALUE);
        }

        constexpr Option<T> ok(this auto&& self) {
            if (self.type != state::ok)
                return None;
            return Some(std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        constexpr Option<E> err(this auto&& self) {
            if (self.type != state::err)
                return None;
            return Some(std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        template <typename TV>
        constexpr Result<TV, E> ok_to(this auto&& self) {
            if (self.type == state::ok) [[unlikely]]
                debug::panic("Cannot coerce Ok type when populated! Use ::map()");
            return Result<TV, E>(Err(std::forward<decltype(self)>(self).data.ERR_VALUE));
        }

        template <typename EV>
        constexpr Result<T, EV> err_to(this auto&& self) {
            if (self.type == state::err) [[unlikely]]
                debug::panic("Cannot coerce Err type when populated! Use ::map()");
            return Result<T, EV>(Ok(std::forward<decltype(self)>(self).data.OK_VALUE));
        }

        template <typename F>
        constexpr auto map(this auto&& self, F&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.OK_VALUE);
            using U = std::remove_cvref_t<std::invoke_result_t<F&&, Arg>>;

            static_assert(!std::same_as<U, void>, "Result::map trying to produce Result<void, E>");

            if (self.type != state::ok)
                return Result<U, E>(Err(std::forward<Self>(self).data.ERR_VALUE));
            return Result<U, E>(Ok(std::invoke(std::forward<F>(f),
                                               std::forward<Self>(self).data.OK_VALUE)));
        }

        template <typename F>
        constexpr auto map_err(this auto&& self, F&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.ERR_VALUE);
            using U = std::remove_cvref_t<std::invoke_result_t<F&&, Arg>>;

            static_assert(!std::same_as<U, void>, "Result::map_err trying to produce Result<T, void>");

            if (self.type != state::err)
                return Result<T, U>(Ok(std::forward<Self>(self).data.OK_VALUE));
            return Result<T, U>(Err(std::invoke(std::forward<F>(f),
                                                std::forward<Self>(self).data.ERR_VALUE)));
        }

        template <typename D, typename F>
        constexpr auto map_or(this auto&& self, D&& def, F&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.OK_VALUE);
            using U = std::decay_t<std::invoke_result_t<F&&, Arg>>;

            static_assert(!std::same_as<U, void>, "Result::map_or trying to produce void");

            if (self.type != state::ok)
                return static_cast<U>(std::forward<D>(def));
            return std::invoke(std::forward<F>(f),
                               std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        template <typename D, typename F>
        constexpr auto map_or_else(this auto&& self, D&& def, F&& f) {
            using Self = decltype(self);
            using ArgOk = decltype(std::forward<Self>(self).data.OK_VALUE);
            using ArgErr = decltype(std::forward<Self>(self).data.ERR_VALUE);
            using U = std::decay_t<std::invoke_result_t<F&&, ArgOk>>;

            static_assert(!std::same_as<U, void>, "Result::map_or_else trying to produce void");
            static_assert(std::convertible_to<std::invoke_result_t<D&&, ArgErr>, U>,
                          "default function must produce a value convertible to map result");

            if (self.type != state::ok)
                return static_cast<U>(std::invoke(std::forward<D>(def),
                                                  std::forward<Self>(self).data.ERR_VALUE));
            return static_cast<U>(std::invoke(std::forward<F>(f),
                                              std::forward<Self>(self).data.OK_VALUE));
        }

        template <typename F>
        requires std::invocable<F&, const T&>
        constexpr decltype(auto) inspect(this auto&& self, F&& f) {
            if (self.type == state::ok)
                std::invoke(std::forward<F>(f), static_cast<const T&>(self.data.OK_VALUE));
            return std::forward<decltype(self)>(self);
        }

        template <typename F>
        requires std::invocable<F&, const E&>
        constexpr decltype(auto) inspect_err(this auto&& self, F&& f) {
            if (self.type == state::err)
                std::invoke(std::forward<F>(f), static_cast<const E&>(self.data.ERR_VALUE));
            return std::forward<decltype(self)>(self);
        }

        constexpr decltype(auto) expect(this auto&& self, std::string_view msg) {
            if (self.type != state::ok) [[unlikely]]
                debug::panic(msg);
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        constexpr decltype(auto) unwrap(this auto&& self) {
            if (self.type != state::ok) [[unlikely]]
                debug::panic("called `Result::unwrap()` on an `Err` value");
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        constexpr decltype(auto) expect_err(this auto&& self, std::string_view msg) {
            if (self.type != state::err) [[unlikely]]
                debug::panic(msg);
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        constexpr decltype(auto) unwrap_err(this auto&& self) {
            if (self.type != state::err) [[unlikely]]
                debug::panic("called `Result::unwrap_err()` on an `Ok` value");
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        template <typename D>
        requires std::constructible_from<T, D&&>
        constexpr T unwrap_or(this auto&& self, D&& def) {
            if (self.type != state::ok)
                return T(std::forward<D>(def));
            return std::forward<decltype(self)>(self).data.OK_VALUE;
        }

        template <typename F>
        constexpr T unwrap_or_else(this auto&& self, F&& f)
            requires std::invocable<F&&, decltype(std::forward<decltype(self)>(self).data.ERR_VALUE)> &&
                     std::constructible_from<T, std::invoke_result_t<F&&, decltype(std::forward<decltype(self)>(self).data.ERR_VALUE)>>
        {
            if (self.type != state::ok)
                return std::invoke(std::forward<F>(f),
                                   std::forward<decltype(self)>(self).data.ERR_VALUE);
            return std::forward<decltype(self)>(self).data.OK_VALUE;
        }

        constexpr T unwrap_or_default(this auto&& self)
            requires (std::is_default_constructible_v<T>)
        {
            if (self.type != state::ok)
                return T{};
            return std::forward<decltype(self)>(self).data.OK_VALUE;
        }

        constexpr decltype(auto) unwrap_unchecked(this auto&& self) noexcept {
            return (std::forward<decltype(self)>(self).data.OK_VALUE);
        }

        constexpr decltype(auto) unwrap_err_unchecked(this auto&& self) noexcept {
            return (std::forward<decltype(self)>(self).data.ERR_VALUE);
        }

        template <typename R>
        requires __detail::result_like<R> &&
                 (!std::same_as<__detail::result_like_ok_t<R>, void>) &&
                 (std::same_as<__detail::result_like_err_t<R>, void> ||
                  std::same_as<__detail::result_like_err_t<R>, E>)
        constexpr auto and_(this auto&& self, R&& res) {
            using U = __detail::result_like_ok_t<R>;
            if (self.type != state::ok)
                return Result<U, E>(Err(std::forward<decltype(self)>(self).data.ERR_VALUE));
            return __detail::to_result<U, E>(std::forward<R>(res));
        }

        template <typename F>
        constexpr auto and_then(this auto&& self, F&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.OK_VALUE);
            using R = std::invoke_result_t<F&&, Arg>;

            static_assert(__detail::result_like<R>, "Result::and_then callback "
                                                    "must return Result, Ok() or Err()");

            if constexpr (std::same_as<__detail::result_like_ok_t<R>, void>) {
                static_assert(std::same_as<__detail::result_like_err_t<R>, E>,
                              "Result::and_then callback error type must match Result error type");
                if (self.type != state::ok)
                    return Result<T, E>(Err(std::forward<Self>(self).data.ERR_VALUE));
                return __detail::to_result<T, E>(std::invoke(std::forward<F>(f),
                                                 std::forward<Self>(self).data.OK_VALUE));
            } else {
                using U = __detail::result_like_ok_t<R>;
                static_assert(std::same_as<__detail::result_like_err_t<R>, void> ||
                              std::same_as<__detail::result_like_err_t<R>, E>,
                              "Result::and_then callback error type must match Result error type");

                if (self.type != state::ok)
                    return Result<U, E>(Err(std::forward<Self>(self).data.ERR_VALUE));
                return __detail::to_result<U, E>(std::invoke(std::forward<F>(f),
                                                 std::forward<Self>(self).data.OK_VALUE));
            }
        }

        template <typename R>
        requires __detail::result_like<R> &&
                 (std::same_as<__detail::result_like_ok_t<R>, void> ||
                  std::same_as<__detail::result_like_ok_t<R>, T>)
        constexpr auto or_(this auto&& self, R&& res) {
            using R_err = __detail::result_like_err_t<R>;
            using F = std::conditional_t<std::same_as<R_err, void>, E, R_err>;

            if (self.type == state::ok)
                return Result<T, F>(Ok(std::forward<decltype(self)>(self).data.OK_VALUE));
            return __detail::to_result<T, F>(std::forward<R>(res));
        }

        template <typename F>
        constexpr auto or_else(this auto&& self, F&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.ERR_VALUE);
            using R = std::invoke_result_t<F&&, Arg>;

            static_assert(__detail::result_like<R>, "Result::or_else callback "
                                                    "must return Result, Ok() or Err()");

            using R_err = __detail::result_like_err_t<R>;
            using OutErr = std::conditional_t<std::same_as<R_err, void>, E, R_err>;

            static_assert(std::same_as<__detail::result_like_ok_t<R>, void> ||
                          std::same_as<__detail::result_like_ok_t<R>, T>,
                          "Result::or_else callback ok type must match Result ok type");

            if (self.type == state::ok)
                return Result<T, OutErr>(Ok(std::forward<Self>(self).data.OK_VALUE));
            return __detail::to_result<T, OutErr>(std::invoke(std::forward<F>(f),
                                                 std::forward<Self>(self).data.ERR_VALUE));
        }

        // TODO:
        // transpose, flatten, copied, cloned
    };
}
