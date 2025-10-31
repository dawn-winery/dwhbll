#pragma once

#include <version>

#if __cpp_impl_reflection >= 202506L 

#include <dwhbll/utils/utils.hpp>
#include <dwhbll/utils/json.hpp>
#include <dwhbll/console/Logging.h>
#include <cassert>
#include <meta>

namespace dwhbll::debug {

constexpr std::string get_indentation(int depth, int step) {
    return std::string(depth * step, ' ');
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string debugfmt_print_struct(T const& val, int depth, int step);

template <typename T>
constexpr std::string debugfmt_print_value(T const& val, int depth, int step) {
    if constexpr (std::formattable<T, char>)
        return std::format("{}", val);
    if constexpr (std::is_class_v<T>)
        return debugfmt_print_struct<T>(val, depth, step);

    // TODO: Check for badly printed stdlib structs and print them
    //       Maybe also implement optional custom formatting
    return "TODO";
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string debugfmt_print_struct(T const &val, int depth, int step) {
    constexpr auto ctx = std::meta::access_context::current();
    constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));

    if constexpr (members.empty())
        return "{}";

    constexpr bool small_struct = [members] constexpr -> bool {
        if constexpr (members.size() > 3)
            return false;

        template for(constexpr auto m : members) {
            using V = [: std::meta::type_of(m) :];
            if constexpr (std::is_class_v<V>) {
                return false;
            }
        }

        return true;
    }();

    if constexpr (small_struct) {
        std::string res = "{ ";
        auto delim = [first=true, &res]() mutable {
            if (!first) res += ", ";
            first = false;
        };

        template for(constexpr auto m : members) {
            using V = [: std::meta::type_of(m) :];
            std::string_view id = std::meta::identifier_of(m);
            delim();
            res += std::format("{} = {}", id, debugfmt_print_value<V>(val.[:m:], depth, step));
        }

        res += " }";
        return res;
    } else {
        std::string res = "{\n";
        std::string ind = get_indentation(depth + 1, step);

        auto delim = [first=true, &res]() mutable {
            if (!first) res += ",\n";
            first = false;
        };

        template for(constexpr auto m : members) {
            using V = [: std::meta::type_of(m) :];
            std::string_view id = std::meta::identifier_of(m);
            delim();
            res += std::format("{}{} = {}", ind, id, debugfmt_print_value<V>(val.[:m:], depth + 1, step));
        }

        res += "\n" + get_indentation(depth, step) + "}";
        return res;
    }
}

template <typename T>
constexpr std::string debugfmt(T const& val, int depth = 0, int step = 4, bool first_indent = true) {
    std::string indent = get_indentation(depth, step);
    constexpr std::string_view type = std::meta::display_string_of(^^T);
    if(first_indent)
        return std::format("{}{} {}", indent, type, debugfmt_print_value(val, depth, step));
    else
        return std::format("{} {}", type, debugfmt_print_value(val, depth, step));
}

#define TRACE_FUNC(func) \
    constexpr std::string_view __id = std::meta::identifier_of(^^func); \
    std::string __s; \
    template for (constexpr auto __e : std::define_static_array(std::meta::parameters_of(^^func))) { \
        using __T = [: std::meta::type_of(__e) :]; \
        __s += std::format("{}{} = {}\n", get_indentation(2, 5), std::meta::identifier_of(__e), debugfmt([: std::meta::variable_of(__e) :], 2, 5, false)); \
    } \
    trace(std::format("function {}:\n{}", __id, __s));


} // namespace dwhbll::debug

#endif
