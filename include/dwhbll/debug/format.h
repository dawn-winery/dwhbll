#pragma once

#include <format>
#include <string_view>
#include <version>

#if __cpp_impl_reflection >= 202506L

#include <meta>
#include <dwhbll/json/json.h>
#include <dwhbll/stl_ext/cow.h>
#include <dwhbll/stl_ext/option.h>
#include <dwhbll/stl_ext/result.h>
#include <dwhbll/unicode/helpers.h>
#include <cassert>
#include <tuple>
#include <vector>

namespace dwhbll::console {
void trace(const std::string& msg);
}

namespace dwhbll::meta {

constexpr std::string get_indentation(int depth, int step) {
    return std::string(depth * step, ' ');
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string dbg_print_struct(T const& val, int depth, int step, bool pretty);

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

// ranges std::format would print as [..] / {..}; excludes self-referential ranges like std::filesystem::path
template <typename T>
concept std_range = std::ranges::input_range<T> && std::ranges::input_range<const T> &&
    (std::format_kind<T> != std::range_format::disabled);

constexpr std::size_t line_width = 80;

template <typename T>
std::string group(std::string_view head, std::string_view open, std::string_view close,
                  T&& items, int depth, int step, bool pretty) {
    const std::vector<std::string> flat = items(false, depth);
    if (flat.empty())
        return head.empty() ? std::format("{}{}", open, close) : std::string(head);

    const bool spaced = !head.empty() && open == "{"; 
    const std::string start = std::format("{}{}{}", head, spaced ? " " : "", open);

    std::string s = start + (spaced ? " " : "");
    for (std::size_t i = 0; i < flat.size(); ++i)
        s += (i ? ", " : "") + flat[i];
    s += std::format("{}{}", spaced ? " " : "", close);
    if (!pretty || depth * step + s.size() <= line_width)
        return s;

    std::string out = start + "\n";
    for (const auto& item : items(true, depth + 1))
        out += get_indentation(depth + 1, step) + item + ",\n";
    return out + get_indentation(depth, step) + std::string(close);
}

template <typename T>
constexpr std::string dbg_print_value(T const& val, int depth, int step, bool pretty) {
    constexpr std::string_view type = std::meta::display_string_of(^^T);

    auto wrap = [&](std::string_view head, auto const& inner) {
        return group(head, "(", ")", [&](bool p, int d) {
            return std::vector{dbg_print_value(inner, d, step, p)};
        }, depth, step, pretty);
    };

    if constexpr (std::same_as<T, char>)
        return std::format("{:?}", val);

    else if constexpr (std::convertible_to<T, std::string_view>) {
        if constexpr (std::is_pointer_v<T>)
            if (!val) return "nullptr"; // string_view(nullptr) would strlen a null pointer
        return std::format("{:?}", std::string_view(val)); // want strings to be quoted
    }

    else if constexpr (std::convertible_to<T const&, std::wstring_view>) {
        const auto code_points = unicode::decode_wide(std::wstring_view(val));
        return std::format("{:?}", unicode::to_utf8(code_points));
    }
    else if constexpr (std::convertible_to<T const&, std::u32string_view>) {
        return std::format("{:?}", unicode::to_utf8(std::u32string_view(val)));
    }
    else if constexpr (std::convertible_to<T const&, std::u16string_view>) {
        return std::format("{:?}", unicode::to_utf8(unicode::decode_utf16(std::u16string_view(val))));
    }
    else if constexpr (std::convertible_to<T const&, std::u8string_view>) {
        return std::format("{:?}", std::string_view(reinterpret_cast<const char*>(val.data()), val.size()));
    }
    // DO NOT MOVE THIS BELOW FORMATTABLE BECAUSE SOME GENIUS MADE UNBOUNDED VIEW FORMATTABLE
    else if constexpr (unbounded_range<T>)
        return "[<unbounded range>]";

    else if constexpr (is_specialization_of<T, ^^stl_ext::Option>())
        return val.is_some() ? wrap("Some", val.unwrap()) : "None";
    else if constexpr (is_specialization_of<T, ^^stl_ext::Result>()) {
        if constexpr (std::is_void_v<decltype(val.unwrap())>) {
            if (val.is_ok()) return "Ok(())";
        } else if (val.is_ok())
            return wrap("Ok", val.unwrap());
        return val.is_err() ? wrap("Err", val.unwrap_err()) : "<invalid Result>";
    }
    else if constexpr (is_specialization_of<T, ^^stl_ext::cow>()) {
        const auto* ptr = val.operator->();
        return ptr ? dbg_print_value(*ptr, depth, step, pretty) : "<empty cow>";
    }
    else if constexpr (is_specialization_of<T, ^^std::optional>()) {
        return val
            ? wrap("Some", *val)
            : "None";
    }
    else if constexpr (is_specialization_of<T, ^^std::variant>()) {
        if (val.valueless_by_exception())
            return "Variant(invlid state)";

        return std::visit([&](const auto& x) {
            return wrap("Variant", x);
        }, val);
    }
    else if constexpr (std_range<T>)
    {
        constexpr auto kind = std::format_kind<T>;

        return group("", kind == std::range_format::sequence ? "[" : "{",
                         kind == std::range_format::sequence ? "]" : "}", [&](bool p, int d) {
            std::vector<std::string> s;
            for (auto &&x: val){
                if constexpr (kind == std::range_format::map) {
                    const auto& [k, v] = x;
                    s.push_back(std::format("{}: {}", dbg_print_value(k, d, step, p), dbg_print_value(v, d, step, p)));
                } else
                    s.push_back(dbg_print_value(x, d, step, p));
            }
            return s;
        }, depth, step, pretty);
    }
    else if constexpr (is_specialization_of<T, ^^std::unique_ptr>() ||
                       is_specialization_of<T, ^^std::shared_ptr>()) {
        // T[] have no deref and will cause an epic segfault if the following is removed
        if constexpr (!requires { *val; })
            return std::format("{}", static_cast<const void*>(val.get()));
        else
            return val ? dbg_print_value(*val, depth, step, pretty) : "nullptr";
    }
    else if constexpr (is_specialization_of<T, ^^std::weak_ptr>()) {
        auto ptr = val.lock();
        if (ptr)
            return dbg_print_value(*ptr, depth, step, pretty);

        return "dead_ptr";
    }
    else if constexpr (is_specialization_of<T, ^^std::pair>() || is_specialization_of<T, ^^std::tuple>())
        return group("", "(", ")", [&](bool p, int d) {
            return std::apply([&](const auto&... f) { return std::vector<std::string>{dbg_print_value(f, d, step, p)...}; }, val);
        }, depth, step, pretty);

    else if constexpr (std::formattable<const T, char>)
        return std::format("{}", val);
    else if constexpr (std::is_pointer_v<T> && std::is_object_v<std::remove_pointer_t<T>>)
        return std::format("{}", static_cast<const void*>(val));
    else if constexpr (std::is_class_v<T>)
        return dbg_print_struct<T>(val, depth, step, pretty);
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
    return std::format("<unprintable `{}`>", type);
}

template <typename T>
requires std::is_class_v<T>
constexpr std::string dbg_print_struct(T const &val, int depth, int step, bool pretty) {
    constexpr std::string_view type = std::meta::display_string_of(^^T);
    
    return group(type, "{", "}", [&]([[maybe_unused]] bool p, [[maybe_unused]] int d) {
        constexpr auto ctx = std::meta::access_context::current();
        std::vector<std::string> res;
        
        template for(constexpr auto m : define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx))) {
            using V = [: std::meta::type_of(m) :];
            std::string_view id = std::meta::identifier_of(m);
            res.push_back(std::format("{}: {}", id, dbg_print_value<V>(val.[:m:], d, step, p)));
        }

        return res;
    }, depth, step, pretty);
}

template <typename T>
constexpr std::string dbg(T const& val, bool pretty = true, int step = 4) {
    return dbg_print_value(val, 0, step, pretty);
}

#define LOG_FUNC(func) \
    constexpr std::string_view __id = std::meta::identifier_of(^^func); \
    std::string __s; \
    template for (constexpr auto __e : std::define_static_array(std::meta::parameters_of(^^func))) { \
        using __T = [: std::meta::type_of(__e) :]; \
        __s += std::format("{}{} = {}\n", ::dwhbll::meta::get_indentation(2, 5), std::meta::identifier_of(__e), ::dwhbll::meta::dbg_print_value([: std::meta::variable_of(__e) :], 2, 5, true)); \
    } \
    ::dwhbll::console::trace(std::format("function {}:\n{}", __id, __s));

} // namespace dwhbll::meta

#else

namespace dwhbll::meta {
template <typename T>
constexpr std::string dbg(T const& val, bool = true, int = 4) {
    return std::format("{}", val);
}
}

#endif

#define 🦀 dbg
// formatters for stl_ext
#include <dwhbll/debug/formatters/option.h>
#include <dwhbll/debug/formatters/result.h>
#include <dwhbll/debug/formatters/cow.h>
#include <dwhbll/debug/formatters/enum.h>
