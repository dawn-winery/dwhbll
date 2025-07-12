#pragma once

#ifdef DWHBLL_REFLECTION
#include <experimental/meta>
#endif

namespace dwhbll::utils {

#ifdef DWHBLL_REFLECTION
template<typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
requires std::is_enum_v<E>
constexpr std::string_view enum_to_string(E value) {
    if constexpr (Enumerable) {
        template for (constexpr auto e :
                      std::define_static_array(std::meta::enumerators_of(^^E))) {
            if (value == [:e:])
                return std::meta::identifier_of(e);
        }
    }

  return "<unnamed>";
}
#endif

} // namespace dwhbll::utils
