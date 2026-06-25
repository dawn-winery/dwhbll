#pragma once

#include <concepts>
#include <functional>
#include <dwhbll/collections/streams.hpp>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/stl_ext/common_helpers.h>

// TODO: Spend 5 afternoons reading the C++ spec and figuring out how to optimize this garbage to be more user friendly.
// TODO: Function template to accept universal
namespace dwhbll::stl_ext {
    template <typename T, typename E>
    requires (!std::same_as<T, void> && !std::same_as<E, void>)
    class Result;

    /**
     * @brief rust but in c++ I mean what?
     * @tparam T Value of Some variant
     */
    template <typename T>
    requires (!std::same_as<T, void>)
    class Option {
        enum TYPE {
            None,
            Some,
        } type;

        struct DUMMY_TYPE_NEVER{};
        union DATA {
            DUMMY_TYPE_NEVER NEVER{};
            T SOME_VALUE;

            ~DATA() {}
        } data;

        void __destroy_storage() {
            if (type == Some)
                data.SOME_VALUE.~T();
        }

    public:
        template <typename TV>
        requires (std::is_convertible_v<TV, T>)
        Option(const __detail::result_some_helper<TV>&& some_val) {
            type = Some;
            new (&data.SOME_VALUE) T(std::move(some_val.value));
        }

        Option(const __detail::result_none_helper&& none_val) {
            type = None;
        }

        template <typename TV>
        requires (std::is_convertible_v<TV, T>)
        Option(const TV&& ok_val) :
            Option(__detail::result_some_helper<T>(std::move(ok_val))) {}

        Option() :
            Option(__detail::result_none_helper()) {}

        Option(const Option &other) {
            type = other.type;
            if (type == Some)
                new (&data.SOME_VALUE) T(other.data.SOME_VALUE);
        }

        Option(Option &&other) noexcept {
            type = other.type;
            if (type == Some)
                new (&data.SOME_VALUE) T(std::move(other.data.SOME_VALUE));
        }

        Option & operator=(const Option &other) {
            if (this == &other)
                return *this;
            __destroy_storage();
            type = other.type;
            if (type == Some)
                new (&data.SOME_VALUE) T(other.data.SOME_VALUE);
            return *this;
        }

        Option & operator=(Option &&other) noexcept {
            if (this == &other)
                return *this;
            __destroy_storage();
            type = other.type;
            if (type == Some)
                new (&data.SOME_VALUE) T(std::move(other.data.SOME_VALUE));
            return *this;
        }

        ~Option() {
            __destroy_storage();
        }

        [[nodiscard]] constexpr bool is_some() const noexcept {
            return type == Some;
        }

        [[nodiscard]] constexpr bool is_some_and(const std::function<bool(T)> &f) const noexcept {
            return type == Some && f(data.SOME_VALUE);
        }

        [[nodiscard]] constexpr bool is_none() const noexcept {
            return type == None;
        }

        [[nodiscard]] constexpr bool is_none_or(const std::function<bool(T)> &f) const noexcept {
            return type == None || f(data.SOME_VALUE);
        }

        decltype(auto) expect(this auto&& self, const std::string &msg) {
            if (self.type == None)
                debug::panic(msg);
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        decltype(auto) unwrap(this auto&& self) {
            if (self.type == None)
                debug::panic("called `Option::unwrap()` on a `None` value");
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        decltype(auto) unwrap_or(this auto&& self, const T&& def) {
            if (self.type == None)
                return def;
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        decltype(auto) unwrap_or_else(this auto&& self, const std::function<T()> &f) {
            if (self.type == None)
                return f();
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        decltype(auto) unwrap_or_default(this auto&& self)
            requires (std::is_default_constructible_v<T>) {
            if (self.type == None)
                return T{};
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        decltype(auto) unwrap_unchecked(this auto&& self) {
            return (std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename U>
        Option<U> map(this auto&& self, const std::function<U(T)> &f) {
            if (self.type == None)
                return Option<U>();
            return Option<U>(f(std::forward<decltype(self)>(self).data.SOME_VALUE));
        }

        decltype(auto) inspect(this auto&& self, const std::function<void(T)> &f) {
            if (self.type == Some)
                f(self.data.SOME_VALUE);
            return self;
        }

        template <typename U>
        U map_or(this auto&& self, const U&& def, const std::function<U(T)> &f) {
            if (self.type == None)
                return def;
            return f(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename U>
        U map_or_else(this auto&& self, const std::function<U()> &def, const std::function<U(T)> &f) {
            if (self.type == None)
                return def();
            return f(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename E>
        Result<T, E> ok_or(this auto&& self, const E&& err) {
            if (self.type == None)
                return Err(err);
            return Ok(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename E>
        Result<T, E> ok_or_else(this auto&& self, const std::function<E()>& err) {
            if (self.type == None)
                return Err(err());
            return Ok(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        template <typename U>
        Option<U> and_(this auto&& self, const Option<U>&& optb) {
            if (self.type == None)
                return Option<U>();
            return optb;
        }

        template <typename U>
        Option<U> and_then(this auto&& self, const std::function<Option<U>(T)>& f) {
            if (self.type == None)
                return Option<U>();
            return f(std::forward<decltype(self)>(self).data.SOME_VALUE);
        }

        Option filter(this auto&& self, const std::function<bool(T&)> &predicate) {
            if (self.type == None)
                return Option();
            if (predicate(self.data.SOME_VALUE))
                return (std::forward<decltype(self)>(self).data.SOME_VALUE);
            return Option();
        }

        Option or_(this auto&& self, const Option&& optb) {
            if (self.type == Some)
                return *self;
            return optb;
        }

        Option or_else(this auto&& self, const std::function<Option()> &f) {
            if (self.type == Some)
                return *self;
            return f();
        }

        Option xor_(this auto&& self, const Option&& optb) {
            if (!(self.type == Some ^ optb.type == Some))
                return Option();
            if (self.type == Some)
                return *self;
            return optb;
        }

        // TODO:
        // insert, get_or_insert, get_or_insert_default, get_or_insert_with, take, take_if, replace, zip, unzip,
        // transpose, flatten,
    };
}
