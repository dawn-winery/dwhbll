#pragma once

#include <dwhbll/utils/utils.hpp>
#include <dwhbll/utils/json.hpp>
#include <dwhbll/console/Logging.h>
#include <cassert>


namespace dwhbll::debug {

constexpr std::string get_indentation(int depth, int step) {
    return std::string(depth * step, ' ');
}

#ifdef DWHBLL_REFLECTION
template <typename T>
requires std::is_class_v<T>
constexpr std::string debugfmt_print_struct(T* val, int depth, int step);

template <typename T>
constexpr std::string debugfmt_print_value(T* val, int depth, int step) {
    if constexpr (std::formattable<T, char>)
        return std::format("{}", *val);
    if constexpr (std::is_class_v<T>)
        return debugfmt_print_struct<T>(val, depth, step);

    // TODO: Check for badly printed std structs and print them
    //       Maybe also implement optional custom formatting
    return "TODO";
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string debugfmt_print_struct(T* val, int depth, int step) {
    std::string res = "{\n";
    std::string ind = get_indentation(depth + 1, step);

    constexpr auto ctx = std::meta::access_context::current();
    template for(constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx))) {
        std::string_view type = std::meta::display_string_of(std::meta::type_of(m));
        std::string_view id = std::meta::identifier_of(m);
        res += std::format("{}{} {} = {}\n", ind, type, id, debugfmt_print_value(&val->[: m :], depth + 1, step));
    }

    std::string short_ind = get_indentation(depth, step);
    res += short_ind + "}";
    return res;
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string debugfmt(T* val, int step = 4) {
    std::string indent = get_indentation(0, step);
    constexpr std::string_view type = std::meta::display_string_of(^^T);
    return std::format("{}{} {}", indent, type, debugfmt_print_struct(val, 0, step));
}
#endif

} // namespace dwhbll::debug
