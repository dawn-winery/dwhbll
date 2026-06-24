#pragma once

#include <utility>

namespace dwhbll::stl_ext {
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

        struct err_value_helper {
            template <typename T>
            result_err_helper<T> operator()(T&& data) {
                return result_err_helper(std::forward<T>(data));
            }
        };

        struct ok_value_helper {
            template <typename T>
            result_ok_helper<T> operator()(T&& data) {
                return result_ok_helper(std::forward<T>(data));
            }
        };

        struct none_value_helper {
            result_none_helper operator()() const {
                return {};
            }
        };
    }

    inline __detail::err_value_helper Err;
    inline __detail::ok_value_helper Ok;
    inline __detail::none_value_helper None;
}
