#pragma once

#include <concepts>
#include <functional>

#include <dwhbll/debug/debug.h>
#include <dwhbll/stl_ext/common_helpers.h>

// TODO: Spend 5 afternoons reading the C++ spec and figuring out how to optimize this garbage to be more user friendly.
namespace dwhbll::stl_ext {
    template <typename T, typename E>
    requires (!std::same_as<T, void> && !std::same_as<E, void>)
    class Result;

    template <typename T>
    requires (!std::same_as<T, void>)
    class Option;

    namespace __detail {
        template<typename T>
        struct option_like_traits {
            static constexpr bool value = false;
        };

        template<typename T>
        struct option_like_traits<Option<T>> {
            static constexpr bool value = true;
            using TYPE = T;
        };

        template<typename T>
        struct option_like_traits<result_some_helper<T>> {
            static constexpr bool value = true;
            using TYPE = T;
        };

        template<typename T>
        concept option_like = option_like_traits<std::remove_cvref_t<T>>::value;

        template<typename T>
        using option_like_value_t = typename option_like_traits<std::remove_cvref_t<T>>::TYPE;

        template <typename T>
        constexpr Option<T> to_option(Option<T>&& value) {
            return std::move(value);
        }

        template <typename T>
        constexpr Option<T> to_option(const Option<T>& value) {
            return value;
        }

        template <typename T>
        constexpr Option<T> to_option(result_some_helper<T>&& value) {
            return Some(std::move(value.value));
        }

        template <typename T>
        constexpr Option<T> to_option(const result_some_helper<T>& value) {
            return Some(value.value);
        }

        template <typename T>
        constexpr Option<T> to_option(none_value_helper) {
            return None;
        }
    }

    /**
     * @brief rust but in c++ I mean what?
     * @tparam T Value of Some variant
     */
    template <typename T>
    requires (!std::same_as<T, void>)
    class Option {
        // Maybe just replace with a bool?
        // bool has_value
        enum class state : bool {
            none,
            some,
        };

        struct DUMMY_TYPE_NEVER{};
        union DATA {
            DUMMY_TYPE_NEVER NEVER{};
            T SOME_VALUE;

            constexpr ~DATA() {}
        } data;
        state type = state::none;

        constexpr void __destroy_storage() {
            if (type == state::some) {
                std::destroy_at(&data.SOME_VALUE);
                type = state::none;
            }
        }

    public:
        template <typename TV>
        requires std::constructible_from<T, TV>
        constexpr Option(__detail::result_some_helper<TV>&& some_val)
            noexcept(std::is_nothrow_constructible_v<T, TV>)
        {
            std::construct_at(&data.SOME_VALUE, std::move(some_val.value));
            type = state::some;
        }

        constexpr Option(__detail::none_value_helper) noexcept {}

        template <typename TV>
        requires (!__detail::some_helper<TV>) &&
                  std::constructible_from<T, TV&&>
        Option(TV&& value)
            noexcept(std::is_nothrow_constructible_v<T, TV&&>)
        {
            std::construct_at(&data.SOME_VALUE, std::forward<TV>(value));
            type = state::some;
        }

        constexpr Option() noexcept {}

        constexpr Option(const Option &other)
            requires std::copy_constructible<T>
        {
            if (other.type == state::some) {
                std::construct_at(&data.SOME_VALUE, other.data.SOME_VALUE);
                type = state::some;
            }
        }

        constexpr Option(Option &&other)
            noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::move_constructible<T>
        {
            if (other.type == state::some) {
                std::construct_at(&data.SOME_VALUE, std::move(other.data.SOME_VALUE));
                type = state::some;
            }
        }

        // TODO: if T is not assignable, destory and reconstruct
        constexpr Option & operator=(const Option &other)
            requires std::copy_constructible<T> &&
                     std::is_copy_assignable_v<T>
        {
            if (this == &other)
                return *this;
            if (other.type == state::none) {
                __destroy_storage();
                return *this;
            }

            if (type == state::some)
                data.SOME_VALUE = other.data.SOME_VALUE;
            else {
                std::construct_at(&data.SOME_VALUE, other.data.SOME_VALUE);
                type = state::some;
            }

            return *this;
        }

        constexpr Option & operator=(Option &&other)
            noexcept(std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_move_assignable_v<T>)
            requires std::move_constructible<T> &&
                     std::is_move_assignable_v<T>
        {
            if (this == &other)
                return *this;
            if (other.type == state::none) {
                __destroy_storage();
                return *this;
            }

            if (type == state::some)
                data.SOME_VALUE = std::move(other.data.SOME_VALUE);
            else {
                std::construct_at(&data.SOME_VALUE, std::move(other.data.SOME_VALUE));
                type = state::some;
            }

            return *this;
        }

        ~Option() {
            __destroy_storage();
        }

        [[nodiscard]] constexpr bool is_some() const noexcept {
            return type == state::some;
        }

        template <typename F>
        requires std::invocable<F&&, const T&> &&
                 std::convertible_to<std::invoke_result_t<F&&, const T&>, bool>
        [[nodiscard]] constexpr bool is_some_and(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F&&, const T&>)
        {
            return type == state::some && std::invoke(std::forward<F>(f), data.SOME_VALUE);
        }

        [[nodiscard]] constexpr bool is_none() const noexcept {
            return type == state::none;
        }

        template <typename F>
        requires std::invocable<F&&, const T&> &&
                 std::convertible_to<std::invoke_result_t<F&&, const T&>, bool>
        [[nodiscard]] constexpr bool is_none_or(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F&&, const T&>)
        {
            return type == state::none || std::invoke(std::forward<F>(f), data.SOME_VALUE);
        }

        constexpr decltype(auto) expect(this auto&& self, std::string_view msg) {
            if (self.type == state::none) [[unlikely]]
                debug::panic(msg);
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        constexpr decltype(auto) unwrap(this auto&& self) {
            if (self.type == state::none) [[unlikely]]
                debug::panic("called `Option::unwrap()` on a `None` value");
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename D>
        requires std::constructible_from<T, D&&>
        constexpr T unwrap_or(this auto&& self, D&& def) {
            if (self.type == state::none)
                return T(std::forward<D>(def));
            return std::forward<decltype(self)>(self).data.SOME_VALUE;
        }

        template <typename F>
        requires std::invocable<F&&> &&
                 std::constructible_from<T, std::invoke_result_t<F&&>>
        constexpr T unwrap_or_else(this auto&& self, F&& f) {
            if (self.type == state::none)
                return std::invoke(std::forward<F>(f));
            return std::forward<decltype(self)>(self).data.SOME_VALUE;
        }

        constexpr T unwrap_or_default(this auto&& self)
            requires (std::is_default_constructible_v<T>)
        {
            if (self.type == state::none)
                return T{};
            return std::forward<decltype(self)>(self).data.SOME_VALUE;
        }

        constexpr decltype(auto) unwrap_unchecked(this auto&& self) noexcept {
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename F>
        constexpr auto map(this auto&& self, F&& f)
        {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.SOME_VALUE);
            using U = std::remove_cvref_t<std::invoke_result_t<F&&, Arg>>;

            static_assert(!std::same_as<U, void>, "Option::map trying to produce Option<void>");

            if (self.type == state::none)
                return Option<U>();
            return Option<U>(std::invoke(std::forward<F>(f),
                                         std::forward<Self>(self).data.SOME_VALUE));
        }

        template <typename F>
        requires std::invocable<F&, const T&>
        constexpr decltype(auto) inspect(this auto&& self, F&& f) {
            if (self.type == state::some)
                std::invoke(std::forward<F>(f), static_cast<const T&>(self.data.SOME_VALUE));
            return std::forward<decltype(self)>(self);
        }

        template <typename D, typename F>
        constexpr auto map_or(this auto&& self, D&& def, F&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.SOME_VALUE);
            using U = std::decay_t<std::invoke_result_t<F&&, Arg>>;

            static_assert(!std::same_as<U, void>, "Option::map_or trying to produce void");

            if (self.type == state::none)
                return static_cast<U>(std::forward<D>(def));
            return std::invoke(std::forward<F>(f),
                               std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename F, typename G>
        constexpr auto map_or_else(this auto&& self, F&& def, G&& f) {
            using Self = decltype(self);
            using Arg = decltype(std::forward<Self>(self).data.SOME_VALUE);
            using U = std::decay_t<std::invoke_result_t<G&&, Arg>>;

            static_assert(!std::same_as<U, void>, "Option::map_or_else trying to produce void");
            static_assert(std::convertible_to<std::invoke_result_t<F&&>, U>,
                          "default function must produce a value convertible to map result");

            if (self.type == state::none)
                return static_cast<U>(std::invoke(std::forward<F>(def)));
            return static_cast<U>(std::invoke(std::forward<G>(f),
                                              std::forward<Self>(self).data.SOME_VALUE));
        }

        template <typename E>
        constexpr Result<T, std::decay_t<E>> ok_or(this auto&& self, E&& err) {
            if (self.type == state::none)
                return Err(std::forward<E>(err));
            return Ok(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename F>
        requires std::invocable<F&&>
        constexpr auto ok_or_else(this auto&& self, F&& err) {
            using E = std::decay_t<std::invoke_result_t<F&&>>;
            if (self.type == state::none)
                return Err(std::invoke(std::forward<F>(err)));
            return Ok(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename O>
        requires __detail::option_like<O> &&
                 std::same_as<__detail::option_like_value_t<O>, T>
        constexpr Option<T> and_(this auto&& self, O&& optb) {
            if (self.type == state::none)
                return None;
            return __detail::to_option<T>(std::forward<O>(optb));
        }

        template <typename F>
        constexpr auto and_then(this auto&& self, F&& f) {
            using Self = decltype(self);
            using Arg = decltype((std::forward<Self>(self).data.SOME_VALUE));
            using R = std::invoke_result_t<F&&, Arg>;

            static_assert(__detail::option_like<R>, "Option::and_then callback "
                                                    "must return Option or Some()");

            using U = __detail::option_like_value_t<R>;
            if (self.type == state::none)
                return Option<U>(None);
            return __detail::to_option<U>(std::invoke(std::forward<F>(f),
                                    std::forward<Self>(self).data.SOME_VALUE));
        }

        template <typename F>
        requires std::invocable<F&&, const T&> &&
                 std::convertible_to<std::invoke_result_t<F&&, const T&>, bool>
        constexpr Option filter(this auto&& self, F&& predicate) {
            if (self.type == state::none)
                return Option();
            if (std::invoke(std::forward<F>(predicate), self.data.SOME_VALUE))
                return Option(std::forward<decltype(self)>(self).data.SOME_VALUE);
            return Option();
        }

        template <typename O>
        requires __detail::option_like<O> &&
                 std::same_as<__detail::option_like_value_t<O>, T>
        constexpr Option<T> or_(this auto&& self, O&& optb) {
            if (self.type == state::some)
                return Option<T>(std::forward<decltype(self)>(self));
            return __detail::to_option<T>(std::forward<O>(optb));
        }

        template <typename F>
        requires std::invocable<F&&>
        constexpr Option<T> or_else(this auto&& self, F&& f) {
            using R = std::invoke_result_t<F&&>;
            if (self.type == state::some)
                return Option<T>(std::forward<decltype(self)>(self));

            // TODO: Probably add None to option_like?
            if constexpr (std::same_as<std::remove_cvref_t<R>, __detail::none_value_helper>)
                return None;
            else {
                static_assert(__detail::option_like<R> &&
                              std::same_as<__detail::option_like_value_t<R>, T>,
                              "Option::or_else callback must return Option or Some(...)");

                return __detail::to_option<T>(std::invoke(std::forward<F>(f)));
            }
        }

        template <typename O>
        requires __detail::option_like<O> &&
                 std::same_as<__detail::option_like_value_t<O>, T>
        constexpr Option<T> xor_(this auto&& self, O&& optb) {
            // TODO: don't do this
            Option<T> other = __detail::to_option<T>(std::forward<O>(optb));
            const bool self_some = self.type == state::some;
            if (self_some == other.is_some())
                return None;
            if (self_some)
                return Option<T>(std::forward<decltype(self)>(self));
            return other;
        }

        // TODO:
        // insert, get_or_insert, get_or_insert_default, get_or_insert_with, take, take_if, replace, zip, unzip,
        // transpose, flatten,
    };
}
