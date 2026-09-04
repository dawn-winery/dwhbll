#pragma once

#include <utility>

namespace dwhbll::stl_ext {
    struct UNIT {};

    constexpr auto TO_UNIT = [](auto) -> UNIT { return {}; };

    namespace __detail {
        template <typename T>
        struct result_err_helper {
            T value;
        };

        template <typename T>
        struct result_ok_helper {
            T value;
        };

        template <typename T>
        struct result_some_helper {
            T value;
        };

        struct err_value_helper {
            template <typename T>
            constexpr result_err_helper<std::decay_t<T>> operator()(T&& data) const {
                return { std::forward<T>(data) };
            }
            constexpr result_err_helper<UNIT> operator()() const {
                return {};
            }
        };

        struct ok_value_helper {
            template <typename T>
            constexpr result_ok_helper<std::decay_t<T>> operator()(T&& data) const {
                return { std::forward<T>(data) };
            }
            constexpr result_ok_helper<UNIT> operator()() const {
                return {};
            }
        };

        struct none_value_helper {
            constexpr none_value_helper operator()() const {
                return {};
            }
        };

        struct some_value_helper {
            template <typename T>
            constexpr result_some_helper<std::decay_t<T>> operator()(T&& data) const {
                return { std::forward<T>(data) };
            }
            constexpr result_some_helper<UNIT> operator()() const {
                return {};
            }
        };

        template <typename T>
        struct is_some_helper : std::false_type {};

        template <typename T>
        struct is_some_helper<result_some_helper<T>> : std::true_type {};
    }

    inline constexpr __detail::err_value_helper Err;
    inline constexpr __detail::ok_value_helper Ok;
    inline constexpr __detail::none_value_helper None;
    inline constexpr __detail::some_value_helper Some;
}
