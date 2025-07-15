#pragma once

#include <string>

#ifdef DWHBLL_REFLECTION
#include <experimental/meta>
#endif

namespace dwhbll::utils {

std::string replace_all(const std::string_view& str, const std::string_view& from, const std::string_view& to);

#ifdef DWHBLL_REFLECTION
template <typename T>
consteval bool has_annotation(std::meta::info r, T const& value) {
    auto expected = reflect_value(value);
    for (std::meta::info a : annotations_of(r)) {
        if (constant_of(a) == expected) {
            return true;
        }
    }
    return false;
}

template<typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
requires std::is_enum_v<E>
consteval std::string_view enum_to_string(E value) {
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
