#pragma once

#include <version>

#if __cpp_impl_reflection >= 202506L
#include <meta>
#include <string>
#include <algorithm>
#include <vector>

namespace dwhbll::utils {

consteval bool has_annotation(std::meta::info r, std::meta::info type) {
    for (auto a : annotations_of(r)) {
        if(std::meta::dealias(std::meta::type_of(a)) == type) {
            return true;
        }
    }
    return false;
}

template<typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
requires std::is_enum_v<E>
constexpr std::string_view enum_to_string(E value) {
    if constexpr (Enumerable) {
        template for (constexpr auto e : define_static_array(std::meta::enumerators_of(^^E))) {
            if (value == [:e:])
                return std::meta::identifier_of(e);
        }
    }

    return "<unnamed>";
}

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
requires std::is_enum_v<E>
constexpr std::optional<E> string_to_enum(std::string_view name) {
    if constexpr (Enumerable) {
        template for (constexpr auto e :
                     define_static_array(std::meta::enumerators_of(^^E))) {
            std::string id = std::string(std::meta::identifier_of(e));
            if (name == id)
                return [:e:];
        }
    }

    return std::nullopt;
}

} // namespace dwhbll::utils
#endif
