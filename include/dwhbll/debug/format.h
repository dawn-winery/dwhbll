#pragma once

#include <format>
#include <version>

#if __cpp_impl_reflection >= 202506L

#include <meta>
#include <dwhbll/json/json.h>
#include <dwhbll/stl_ext/cow.h>
#include <dwhbll/stl_ext/option.h>
#include <dwhbll/stl_ext/result.h>
#include <dwhbll/unicode/helpers.h>
#include <cassert>

namespace dwhbll::console {
void trace(const std::string& msg);
}

namespace dwhbll::meta {

constexpr std::string get_indentation(int depth, int step) {
    return std::string(depth * step, ' ');
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string dbg_print_struct(T const& val, int depth, int step);

template <typename T, std::meta::info Template>
consteval bool is_specialization_of() {
    if constexpr (std::meta::has_template_arguments(^^T))
        return std::meta::template_of(^^T) == Template;
    return false;
}

template <typename T>
concept unbounded_range =
    std::ranges::input_range<const T> &&
    requires {
        requires std::same_as<std::ranges::sentinel_t<const T>,std::unreachable_sentinel_t>;
    };

template <typename T>
constexpr std::string dbg_print_value(T const& val, int depth, int step) {
    constexpr std::string_view type = std::meta::display_string_of(^^T);


    if constexpr (std::convertible_to<T, std::string_view>)
        return std::format("{:?}", std::string_view(val)); // want strings to be quoted

    else if constexpr (std::convertible_to<T const&, std::wstring_view>) {
        const auto code_points = unicode::decode_wide(std::wstring_view(val));
        return std::format("{:?}", unicode::to_utf8(code_points));
    }
    // DO NOT MOVE THIS BELOW FORMATTABLE BECAUSE SOME GENIUS MADE UNBOUNDED VIEW FORMATTABLE
    else if constexpr (unbounded_range<T>)
        return "[<unbounded range>]";

    else if constexpr (std::formattable<T, char>)
        return std::format("{}", val);
    else if constexpr (is_specialization_of<T, ^^std::optional>()) {
        return val
            ? std::format("Some({})", dbg_print_value(*val, depth, step))
            : "None";
    }
    else if constexpr (is_specialization_of<T, ^^std::variant>()) {
        if (val.valueless_by_exception())
            return "Variant(invlid state)";

        return std::visit([&](const auto& x) {
            return std::format("Variant({})", dbg_print_value(x, depth, step));
        }, val);
    }
    else if constexpr (std::ranges::input_range<const T>)
    {
        std::string s{"["};

        auto delim = [first=true, &s]() mutable {
            if (!first) s += ", ";
            first = false;
        };

        for (auto &&x: val){
            delim();
            s+=dbg_print_value(x,depth,step);
        }
        return std::format("{}]",s);
    }
    else if constexpr (is_specialization_of<T, ^^std::unique_ptr>() ||
                       is_specialization_of<T, ^^std::shared_ptr>()) {
        return val
            ? std::format("{}({})", type, dbg_print_value(*val, depth, step))
            : std::format("{}(nullptr)", type);
    }
    else if constexpr (is_specialization_of<T, ^^std::weak_ptr>()) {
        auto ptr = val.lock();
        if (ptr)
            return std::format("{}({})", type, dbg_print_value(*ptr, depth, step));

        return std::format("{}(expired_ptr)", type);
    }
    else if constexpr (std::is_class_v<T>)
        return std::format("{} {}", type, dbg_print_struct<T>(val, depth, step));
    else if constexpr (std::is_enum_v<T>) {
        constexpr auto static enumerators =
            std::define_static_array(std::meta::enumerators_of(^^T));

        template for (constexpr auto e : enumerators) {
            if (val == [:e:])
                return std::string(std::meta::identifier_of(e));
        }
        return std::format("{}", std::to_underlying(val));
    }
    // TODO: Check for badly printed stdlib structs and print them
    //       Maybe also implement optional custom formatting
    return "TODO";
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string dbg_print_struct(T const &val, int depth, int step) {
    constexpr auto ctx = std::meta::access_context::current();
    constexpr auto members = define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));

    if constexpr (members.empty())
        return "{}";

    constexpr bool small_struct = [] constexpr -> bool {
        constexpr auto members = define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));

        if constexpr (members.size() > 3)
            return false;

        template for(constexpr auto m : define_static_array(members)) {
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

        template for(constexpr auto m : define_static_array(members)) {
            using V = [: std::meta::type_of(m) :];
            std::string_view id = std::meta::identifier_of(m);
            delim();
            res += std::format("{} = {}", id, dbg_print_value<V>(val.[:m:], depth, step));
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

        template for(constexpr auto m : define_static_array(members)) {
            using V = [: std::meta::type_of(m) :];
            std::string_view id = std::meta::identifier_of(m);
            delim();
            res += std::format("{}{} = {}", ind, id, dbg_print_value<V>(val.[:m:], depth + 1, step));
        }

        res += "\n" + get_indentation(depth, step) + "}";
        return res;
    }
}

template <typename T>
constexpr std::string dbg(T const& val, int depth = 0, int step = 4, bool first_indent = true) {
    std::string indent = get_indentation(depth, step);
    // constexpr std::string_view type = std::meta::display_string_of(^^T);
    if(first_indent)
        return std::format("{} {}", indent, dbg_print_value(val, depth, step));
    else
        return dbg_print_value(val, depth, step);
}

#define LOG_FUNC(func) \
    constexpr std::string_view __id = std::meta::identifier_of(^^func); \
    std::string __s; \
    template for (constexpr auto __e : std::define_static_array(std::meta::parameters_of(^^func))) { \
        using __T = [: std::meta::type_of(__e) :]; \
        __s += std::format("{}{} = {}\n", get_indentation(2, 5), std::meta::identifier_of(__e), dbg([: std::meta::variable_of(__e) :], 2, 5, false)); \
    } \
    trace(std::format("function {}:\n{}", __id, __s));

} // namespace dwhbll::meta

#else

namespace dwhbll::meta {
template <typename T>
constexpr std::string dbg(T const& val, int depth = 0, int step = 4, bool first_indent = true) {
    return std::format<T>("{}", val);
}
}

#endif

#define 🦀 dbg
// formatters for stl_ext
#include <dwhbll/debug/formatters/option.h>
#include <dwhbll/debug/formatters/result.h>
#include <dwhbll/debug/formatters/cow.h>
