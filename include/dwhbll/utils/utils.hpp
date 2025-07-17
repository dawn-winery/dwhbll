#pragma once

#include <string>
#include <algorithm>

#ifdef DWHBLL_REFLECTION
#include <experimental/meta>
#endif

namespace dwhbll::utils {

constexpr std::string replace_all(std::string_view str, std::string_view from, std::string_view to);
std::string escape_string(std::string_view str);

#ifdef DWHBLL_REFLECTION
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

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
requires std::is_enum_v<E>
consteval std::optional<E> string_to_enum(std::string name, bool case_sensitive = true) {
    if(!case_sensitive)
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if constexpr (Enumerable) {
        template for (constexpr auto e :
                     std::define_static_array(std::meta::enumerators_of(^^E))) {
            std::string id = std::string(std::meta::identifier_of(e));
            if(!case_sensitive)
                std::transform(id.begin(), id.end(), id.begin(), ::tolower);
            if (name == id)
                return [:e:];
        }
    }

    return std::nullopt;
}
#endif

} // namespace dwhbll::utils
