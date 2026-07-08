#pragma once

#include <algorithm>
#include <source_location>
#include <string>
#include <vector>
#include <meta>

namespace dwhbll::test {

struct test_marker {};
inline constexpr test_marker test{};

struct name {
    char const* test_name;

    consteval explicit name(std::string_view name = "") 
        : test_name(std::define_static_string(name)) {}
};

struct skip {
    char const* reason;

    consteval explicit skip(std::string_view reason_ = "") 
        : reason(std::define_static_string(reason_)) {}
};

struct tag {
    char const* tag_name;

    consteval explicit tag(std::string_view name_ = "")
        : tag_name(std::define_static_string(name_)) {}
};

struct failure {
    std::string msg;
    std::source_location loc;
};

namespace detail {

class result {
public:
    void add_failure(std::string msg, std::source_location loc);
    bool passed() const;
    std::vector<failure> const& failures() const;

private:
    std::vector<failure> failures_;
};

extern thread_local result* current_result;

void report_failure(std::string message, std::source_location loc);

struct entry {
    std::string_view name;
    void (*fn)();
    bool skip;
    std::string_view skip_reason;
    std::vector<std::string_view> tags;
};

// Random bullshit go!
template <std::size_t N>
struct fixed_string {
    char data[N]{};
    consteval fixed_string(char const (&s)[N]) {
        std::copy_n(s, N, data);
    }
};
template <std::size_t N> fixed_string(char const (&)[N]) -> fixed_string<N>;

std::vector<entry>& registry();

consteval bool is_test(std::meta::info n) {
    return !std::meta::annotations_of_with_type(n, ^^test_marker).empty();
}

consteval std::meta::info find_annotation(std::meta::info n, std::meta::info type) {
    for (auto a : std::meta::annotations_of_with_type(n, type)) {
        return std::meta::constant_of(a);
    }
    return std::meta::info();
}

template <std::meta::info n>
std::vector<std::string_view> get_tags() {
    std::vector<std::string_view> result;
    template for (constexpr auto a : define_static_array(std::meta::annotations_of_with_type(n, ^^tag))) {
        constexpr auto ann = std::meta::constant_of(a);
        constexpr auto val = extract<tag>(ann);
        result.push_back(std::string_view(val.tag_name));
    }
    return result;
}

template <std::meta::info Scope, fixed_string TU>
void collect_tests() {
    using namespace std::meta;
    constexpr auto ctx = access_context::unchecked();

    template for (constexpr auto n : define_static_array(members_of(Scope, ctx))) {
        if constexpr (is_namespace(n)) {
            if constexpr (!(has_identifier(n) &&
                             (identifier_of(n) == "std" ||
                              identifier_of(n).starts_with("__"))))
            if constexpr (is_enumerable_type(n))
                collect_tests<n, TU>();
        }
        else if constexpr (is_type(n) && is_class_type(n)) {
            if constexpr (is_enumerable_type(n))
                collect_tests<n, TU>();
        }
        else if constexpr (is_function(n)) {
            if constexpr (is_test(n)) {
                static_assert(!is_class_member(n) || is_static_member(n),
                              "test annotation on non-static member functions is not allowed.");

                constexpr auto name_ann = find_annotation(n, ^^name);
                std::string_view name;
                if constexpr (name_ann != info()) {
                    static constexpr auto name_val =
                        extract<typename[: type_of(name_ann) :]>(name_ann);
                    name = std::string_view(name_val.test_name);
                } else {
                    static_assert(has_identifier(n),
                        "test with no name given on a function with no identifier");
                    static constexpr auto id = identifier_of(n);
                    name = id;
                }

                constexpr auto skip_ann = find_annotation(n, ^^skip);
                constexpr bool skip = skip_ann != info();
                std::string_view skip_reason;
                if constexpr (skip) {
                    static constexpr auto skip_val =
                        extract<typename[: type_of(skip_ann) :]>(skip_ann);
                    skip_reason = std::string_view(skip_val.reason);
                }

                auto tags_arr = get_tags<n>();

                auto fn = extract<void(*)()>(n);
                auto& reg = registry();

                bool found = false;
                for (const auto& e : reg) {
                    if (e.fn == fn) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    reg.push_back({ name, fn, skip, skip_reason, tags_arr });
            }
        }
    }
}

} // namespace detail

} // namespace dwhbll::test
