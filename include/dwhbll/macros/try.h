#pragma once

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
