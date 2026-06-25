#pragma once

#include <utility>

namespace dwhbll::stl_ext {
    struct UNIT {};

    namespace __detail {
        template <typename T>
        struct result_err_helper {
            T value;
        };

        template <typename T>
        struct result_ok_helper {
            T value;
        };

        struct result_none_helper {
        };

        template <typename T>
        struct result_some_helper {
            T value;
        };

        struct err_value_helper {
            template <typename T>
            result_err_helper<T> operator()(T&& data) {
                return result_err_helper<T>(std::forward<T>(data));
            }
            result_err_helper<UNIT> operator()() const {
                return {};
            }
        };

        struct ok_value_helper {
            template <typename T>
            result_ok_helper<T> operator()(T&& data) {
                return result_ok_helper<T>(std::forward<T>(data));
            }
            result_ok_helper<UNIT> operator()() const {
                return {};
            }
        };

        struct none_value_helper {
            result_none_helper operator()() const {
                return {};
            }
        };

        struct some_value_helper {
            template <typename T>
            result_some_helper<T> operator()(T&& data) {
                return result_some_helper<T>(std::forward<T>(data));
            }
            result_some_helper<UNIT> operator()() const {
                return {};
            }
        };
    }

    inline __detail::err_value_helper Err;
    inline __detail::ok_value_helper Ok;
    inline __detail::none_value_helper None;
    inline __detail::some_value_helper Some;
}
