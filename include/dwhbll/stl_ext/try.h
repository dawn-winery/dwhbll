#pragma once

#include <dwhbll/stl_ext/option.h>
#include <dwhbll/stl_ext/result.h>

#include <expected>

#define TRY(expr)                                                       \
({                                                                      \
    auto&& _exp = (expr);                                               \
    using _Traits = ::dwhbll::stl_ext::__detail::try_traits<            \
                          std::remove_cvref_t<decltype(_exp)>>;         \
                                                                        \
    if (!_Traits::success(_exp))                                        \
        return _Traits::failure(std::move(_exp));                       \
                                                                        \
    _Traits::value(std::move(_exp));                                    \
})

namespace dwhbll::stl_ext::__detail {

template<typename T>
struct try_traits;

template<typename T, typename E>
struct try_traits<Result<T, E>> {
    static bool success(Result<T, E>& r) {
        return r.is_ok();
    }

    static decltype(auto) value(Result<T, E>&& r) {
        return std::move(r).unwrap_unchecked();
    }

    static auto failure(Result<T, E>&& r) {
        return Err(std::move(r).unwrap_err_unchecked());
    }
};

template<typename T>
struct try_traits<Option<T>> {
    static bool success(Option<T>& o) {
        return o.is_some();
    }

    static decltype(auto) value(Option<T>&& o) {
        return std::move(o).unwrap_unchecked();
    }

    static auto failure(Option<T>&&) {
        return None;
    }
};

template<typename T, typename E>
struct try_traits<std::expected<T, E>> {
    static bool success(std::expected<T, E>& e) {
        return e.has_value();
    }

    static decltype(auto) value(std::expected<T, E>&& e) {
        return std::move(e).value();
    }

    static auto failure(std::expected<T, E>&& e) {
        return std::unexpected(std::move(e).error());
    }
};

} // namespace __detail
