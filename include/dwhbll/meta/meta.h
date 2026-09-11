#pragma once

#include <meta>
#include <algorithm>
#include <string>
#include <string_view>

namespace dwhbll::meta {

// random bs
// Couldn't find a way to get some shit working without this when I was writign
// the testing lib, but now with better gcc reflection impl/lib, it might not be
// necessary anymore.
// TODO: should be removed at some point
template <std::size_t N>
struct fixed_string {
    char data[N]{};
    consteval fixed_string(const char(&s)[N]) {
        std::copy_n(s, N, data);
    }
};
template <std::size_t N> fixed_string(const char(&)[N]) -> fixed_string<N>;

template<typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
requires std::is_enum_v<E>
constexpr std::string_view enum_to_string(E value) {
    if constexpr (Enumerable) {
        template for (constexpr auto e : define_static_array(std::meta::enumerators_of(^^E))) {
            if (value == [:e:])
                return std::meta::identifier_of(e);
        }
    }

    // Maybe return some kind of error/exception at compile time?
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

consteval auto find_annotation(std::meta::info func, std::meta::info type) {
    for (auto a : std::meta::annotations_of_with_type(func, type)) {
        return std::meta::constant_of(a);
    }
    return std::meta::info();
}

template <typename MarkerType>
consteval bool has_annotation(std::meta::info entity) {
    return !std::meta::annotations_of_with_type(entity, ^^MarkerType).empty();
}

// Finds all visible functions in global namespace that have a specific annotation
// and calls Traits::process<func>. Avoids stuff in the std namespace or that start
// with __
template <typename Traits, std::meta::info Scope, fixed_string TU>
void collect_annotated() {
    using namespace std::meta;
    constexpr auto ctx = access_context::unchecked();

    template for (constexpr auto n : define_static_array(members_of(Scope, ctx))) {
        if constexpr (is_namespace(n)) {
            if constexpr (!has_identifier(n) || 
                            (identifier_of(n) != "std" &&
                            !identifier_of(n).starts_with("__")))
                collect_annotated<Traits, n, TU>();
        }
        else if constexpr (is_type(n) && is_class_type(n)) {
            if constexpr (is_enumerable_type(n))
                collect_annotated<Traits, n, TU>();
        }
        else if constexpr (is_function(n)) {
            if constexpr (has_annotation<typename Traits::marker_type>(n))
                Traits::template process<n, Scope>();
        }
    }
}

} // namespace dwhbll::meta
