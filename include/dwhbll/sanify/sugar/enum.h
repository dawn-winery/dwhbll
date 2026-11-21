#pragma once

#include <variant>

#include <dwhbll/sanify/types.h>

namespace dwhbll::sanify::sugar::detail {
    template <typename ParentT, typename TV>
    struct base_variant_tuple {
        TV index_{};

        [[nodiscard]] constexpr TV discriminator() const {
            return index_;
        }

        [[nodiscard]] operator TV() const {
            return index_;
        }

        using parent_t = ParentT;
    };

    template <typename ParentT, typename TV, typename... Args>
    struct variant_tuple : base_variant_tuple<ParentT, TV> {
        std::tuple<std::decay_t<Args>...> tuple;

        template <typename... Args2>
        constexpr variant_tuple(TV index, Args2&&... args) :
            base_variant_tuple<ParentT, TV>(index), tuple(std::forward<Args2>(args)...) {}

        using base_variant_tuple<ParentT, TV>::discriminator;
        using base_variant_tuple<ParentT, TV>::operator TV;
        using typename base_variant_tuple<ParentT, TV>::parent_t;
    };

    template <typename ParentT, typename TV>
    struct variant_tuple<ParentT, TV> : base_variant_tuple<ParentT, TV> {
        using base_variant_tuple<ParentT, TV>::base_variant_tuple;

        using base_variant_tuple<ParentT, TV>::discriminator;
        using base_variant_tuple<ParentT, TV>::operator TV;
        using typename base_variant_tuple<ParentT, TV>::parent_t;
    };

    template <typename T, typename TV, typename... Args>
    struct variant_helper_base {
        constexpr variant_helper_base(TV value) : disc_{value} {}

        template <typename... Args2>
        requires (sizeof...(Args) != 0)
        [[nodiscard]] constexpr T operator()(Args2&&... args) const {
            return T{disc_, args...};
        }

        [[nodiscard]] constexpr T operator()() const requires (sizeof...(Args) == 0)
        {
            return T{disc_};
        }

        [[nodiscard]] constexpr TV discriminant() const {
            return disc_;
        }

        using parent_t = T;

    private:
        TV disc_;
    };

    template <typename T>
    struct is_variant_tuple : std::false_type {};

    template<typename ParentT, typename T, typename... Args>
    struct is_variant_tuple<variant_tuple<ParentT, T, Args...>> : std::true_type {};

    template <typename Self, typename Other>
    concept is_self_tuple = is_variant_tuple<Other>::value && std::is_same_v<typename Other::parent_t, Self>;

    template <typename T>
    struct match_processor {
        constexpr match_processor(const T& data) : ref(data) {}

        constexpr match_processor(const match_processor &other)
            : ref(other.ref), handled(other.handled) {}

        constexpr match_processor(match_processor &&other) noexcept
            : ref(other.ref), handled(other.handled) {}

        constexpr match_processor & operator=(const match_processor &other) = delete;

        constexpr match_processor & operator=(match_processor &&other) noexcept = delete;

        [[nodiscard]] constexpr bool is_handled() const {
            return handled;
        }

        constexpr void set_handled() {
            handled = true;
        }

        constexpr ~match_processor() {
            if (!handled)
                debug::panic(
                    "Unhandled variant!"
                );
        }

        T& ref;

    private:
        bool handled = false;
    };
}

//// ---------- DEFINING VARIANTS ----------

/**
 * @brief begins definition of a new variant.
 * @param name Type name of the ENUM (self reference)
 * @param type Type name of the storage type (e.g. u32)
 */
#define BEGIN_VARIANT(name, type) \
    using self_storage_t = type; \
    using self_t = name

#define ENUM struct

/**
 * @brief define a new variant type.
 * @param name variant name.
 */
#define DEFINE_VARIANT(name, ...) \
    using variant_ ## name = ::dwhbll::sanify::sugar::detail::variant_tuple<self_t, self_storage_t __VA_OPT__(,) __VA_ARGS__>; \
    using variant_helper_ ## name = ::dwhbll::sanify::sugar::detail::variant_helper_base<variant_ ## name, self_storage_t __VA_OPT__(,) __VA_ARGS__>; \
    constexpr static variant_helper_ ## name name

#define __DETAIL_DEFINE_VARIANT_MEMBERS() \
    constexpr storage_t &__impl_storage_t() { return storage_; } \
    constexpr const storage_t &__impl_storage_t() const { return storage_; }

#define __DETAIL_DEFINE_VARIANT_CONSTRUCTORS(self) \
    template <typename T> \
    requires ::dwhbll::sanify::sugar::detail::is_self_tuple<self_t, std::decay_t<T>> \
    constexpr self(T&& body) : storage_{std::forward<T>(body)} {} \

#define __DETAIL_DEFINE_VARIANT_STORAGE(...) \
    using storage_t = std::variant<__VA_ARGS__>; \
private: \
    storage_t storage_; \
public:

#define VARIANT(name) variant_ ## name

/**
 * @brief Completes definition of a variant type, pass all variant types here
 *
 * e.g. VARIANTS(foo, bar, foobar)
 */
#define VARIANTS(self, ...) \
    __DETAIL_DEFINE_VARIANT_STORAGE(__VA_ARGS__) \
    __DETAIL_DEFINE_VARIANT_CONSTRUCTORS(self) \
    __DETAIL_DEFINE_VARIANT_MEMBERS() \

//// ---------- MATCHING ----------

#define MATCH(value) \
    if (::dwhbll::sanify::sugar::detail::match_processor<std::add_lvalue_reference_t<decltype(value)>> \
        __impl_match_processor{value}; true \
    )

#define CASE(type, ...) \
    if (std::holds_alternative<decltype(type)::parent_t>(__impl_match_processor.ref.__impl_storage_t()) && \
        (__impl_match_processor.set_handled(), true)) \
        __VA_OPT__( \
            if (const auto &[__VA_ARGS__] = std::get<decltype(type)::parent_t>(__impl_match_processor.ref.__impl_storage_t()).tuple; true) \
        )

#define CASE_MUT(type, ...) \
    if (std::holds_alternative<decltype(type)::parent_t>(__impl_match_processor.ref.__impl_storage_t()) && \
        (__impl_match_processor.set_handled(), true)) \
        __VA_OPT__( \
            if (auto &[__VA_ARGS__] = std::get<decltype(type)::parent_t>(__impl_match_processor.ref.__impl_storage_t()).tuple; true) \
        )

#define DEFAULT() if (!__impl_match_processor.is_handled() && \
    (__impl_match_processor.set_handled(), true))

#define NONEXHAUSTIVE() __impl_match_processor.set_handled()

#define MAYBE_IS(variable, type, ...) \
    if (std::holds_alternative<decltype(type)::parent_t>(variable.__impl_storage_t())) \
        __VA_OPT__( \
            if (const auto &[__VA_ARGS__] = std::get<decltype(type)::parent_t>(variable.__impl_storage_t()).tuple; true) \
        )

#define MAYBE_IS_MUT(variable, type, ...) \
    if (std::holds_alternative<decltype(type)::parent_t>(variable.__impl_storage_t())) \
        __VA_OPT__( \
            if (auto &[__VA_ARGS__] = std::get<decltype(type)::parent_t>(variable.__impl_storage_t()).tuple; true) \
        )

#define IS_VARIANT(variable, type) \
    (std::holds_alternative<decltype(type)::parent_t>(variable.__impl_storage_t()))
