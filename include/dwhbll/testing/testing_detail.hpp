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
    constexpr explicit name(char const* name = {})
        : test_name(name) {}

    char const* test_name;
};

struct skip {
    constexpr explicit skip(char const* reason_ = {})
        : reason(reason_) {}

    char const* reason;
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
        // This does not work. Why? I truly fucking wonder
        // TODO: dig in draft and figure it out
        // auto c = std::meta::constant_of(a);
        // return c;
    }
    return std::meta::info();
}

template <std::meta::info Scope, fixed_string TU>
void collect_tests() {
    constexpr auto ctx = std::meta::access_context::unchecked();

    template for (constexpr auto n : define_static_array(std::meta::members_of(Scope, ctx))) {
        if constexpr (std::meta::is_namespace(n)) {
            if constexpr (!(std::meta::has_identifier(n) &&
                             (std::meta::identifier_of(n) == "std" ||
                              std::meta::identifier_of(n).starts_with("__"))))
            if constexpr (std::meta::is_enumerable_type(n))
                collect_tests<n, TU>();
        }
        else if constexpr (std::meta::is_type(n) && std::meta::is_class_type(n)) {
            if constexpr (std::meta::is_enumerable_type(n))
                collect_tests<n, TU>();
        }
        else if constexpr (std::meta::is_function(n)) {
            if constexpr (is_test(n)) {
                static_assert(!std::meta::is_class_member(n) || std::meta::is_static_member(n),
                              "test annotation on non-static member functions is not allowed.");

                constexpr auto name_ann = find_annotation(n, ^^name);
                std::string_view name;
                if constexpr (name_ann != std::meta::info()) {
                    static constexpr auto name_val =
                        std::meta::extract<::dwhbll::test::name>(name_ann);
                    name = std::string_view(name_val.test_name);
                } else {
                    static_assert(std::meta::has_identifier(n),
                        "test with no name given on a function with no identifier");
                    static constexpr auto id = std::meta::identifier_of(n);
                    name = id;
                }

                constexpr auto skip_ann = find_annotation(n, ^^skip);
                constexpr bool skip = skip_ann != std::meta::info();
                std::string_view skip_reason;
                if constexpr (skip) {
                    static constexpr auto skip_val =
                        std::meta::extract<::dwhbll::test::skip>(skip_ann);
                    skip_reason = std::string_view(skip_val.reason);
                }

                auto fn = std::meta::extract<void(*)()>(n);
                auto& reg = registry();

                bool found = false;
                for (auto const& e : reg) {
                    if (e.fn == fn) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    reg.push_back({name, fn, skip, skip_reason});
            }
        }
    }
}

} // namespace detail

} // namespace dwhbll::test
