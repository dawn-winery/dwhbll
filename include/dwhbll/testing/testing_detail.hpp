#pragma once

#include <version>

#if __cpp_impl_reflection >= 202506L

#include <source_location>
#include <string>
#include <vector>
#include <meta>

namespace dwhbll::test::detail {

template <std::size_t N>
struct TestInfo {
    char name[N]{};
    bool skip = false;
};

struct failure {
    std::string msg;
    std::source_location loc;
};

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
};

std::vector<entry>& registry();

consteval std::meta::info find_test_annotation(std::meta::info n) {
    for (auto a : std::meta::annotations_of(n)) {
        auto t = std::meta::type_of(a);
        if (std::meta::has_template_arguments(t) &&
            std::meta::template_of(t) == ^^TestInfo) {
            return a;
        }
    }
    return std::meta::info();
}

template <std::meta::info Scope>
void collect_tests() {
    constexpr auto ctx = std::meta::access_context::unchecked();

    template for (constexpr auto n : define_static_array(std::meta::members_of(Scope, ctx))) {
        if constexpr (std::meta::is_namespace(n)) {
            if constexpr (!(std::meta::has_identifier(n) &&
                             (std::meta::identifier_of(n) == "std" ||
                              std::meta::identifier_of(n).starts_with("__"))))
            if constexpr (std::meta::is_enumerable_type(n))
                collect_tests<n>();
        }
        else if constexpr (std::meta::is_type(n) && std::meta::is_class_type(n)) {
            if constexpr (std::meta::is_enumerable_type(n))
                collect_tests<n>();
        }
        else if constexpr (std::meta::is_function(n)) {
            constexpr auto ann = find_test_annotation(n);
            if constexpr (ann != std::meta::info()) {
                if constexpr (std::meta::is_class_member(n) &&
                              !std::meta::is_static_member(n)) {
                    static_assert(false, "Test annotation on non-static member "
                                         "functions is not allowed.");
                }
                else {
                    static constexpr auto info_val =
                        std::meta::extract<typename[: std::meta::type_of(ann) :]>(ann);
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
                        reg.push_back({info_val.name, fn, info_val.skip});
                }
            }
        }
    }
}

inline bool register_file() {
    collect_tests<^^::>();
    return true;
}

} // namespace dwhbll::test::detail

#endif
