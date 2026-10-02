#pragma once

#include <dwhbll/testing/harness.h>
#include <dwhbll/testing/assertions.h>
#include <dwhbll/meta/meta.h>

#include <meta>

namespace dwhbll::test {

struct test_marker {};
inline constexpr test_marker test{};

struct name {
    char const* test_name;

    consteval explicit name(std::string_view name_ = "")
        : test_name(std::define_static_string(name_)) {}
};

struct skip {
    char const* reason;

    consteval explicit skip(std::string_view reason_ = "")
        : reason(std::define_static_string(reason_)) {}
};

struct xfail {
    char const* reason;

    consteval explicit xfail(std::string_view reason_ = "")
        : reason(std::define_static_string(reason_)) {}
};


namespace detail {

struct entry {
    std::string name;
    void (*fn)();
    bool is_skip;
    std::string_view skip_reason;
    bool is_xfail;
    std::string_view xfail_reason;
};

std::vector<entry>& registry();

struct discovery_traits {
    using marker_type = test_marker;

    template <std::meta::info func, std::meta::info scope>
    static constexpr void process() {
        using namespace std::meta;
        if constexpr (annotations_of_with_type(func, ^^test_marker).empty())
            return;

        static_assert(!is_class_member(func) || is_static_member(func),
                      "test annotation on non-static member functions is not allowed.");

        constexpr auto fn = extract<void(*)()>(func);
        auto& reg = registry();

        for (const auto& e : reg) {
            if (e.fn == fn)
                return;
        }

        constexpr auto name_ann = dwhbll::meta::find_annotation(func, ^^name);
        std::string test_name;
        if constexpr (name_ann != info()) {
            static constexpr auto name_val =
                extract<typename[: type_of(name_ann) :]>(name_ann);
            test_name = std::string_view(name_val.test_name);
        } else {
            static_assert(has_identifier(func),
                "test with no name given on a function with no identifier");
            test_name = identifier_of(func);
        }
        if constexpr (has_identifier(scope) && identifier_of(scope) != "::")
            test_name = std::string(identifier_of(scope)) + "/" + test_name;

        // TODO: make this conditional (for example based on architecture)
        //       tbf, the arch check can also be done at build time with
        //       preprocessor so it's not really that important...
        constexpr auto skip_ann = dwhbll::meta::find_annotation(func, ^^skip);
        constexpr bool is_skip = skip_ann != info();
        std::string_view skip_reason;
        if constexpr (is_skip) {
            static constexpr auto skip_val =
                extract<typename[: type_of(skip_ann) :]>(skip_ann);
            skip_reason = std::string_view(skip_val.reason);
        }

        constexpr auto xfail_ann = dwhbll::meta::find_annotation(func, ^^xfail);
        constexpr bool is_xfail = xfail_ann != info();
        std::string_view xfail_reason;
        if constexpr (is_xfail) {
            static constexpr auto xfail_val =
                extract<typename[: type_of(xfail_ann) :]>(xfail_ann);
            xfail_reason = std::string_view(xfail_val.reason);
        }

        reg.push_back({ test_name, fn, is_skip, skip_reason, is_xfail, xfail_reason });
    }
};

} // namespace detail

int run_all(const options& options = {});

} // namespace dwhbll::test

#define TEST_REGISTER_FILE() \
    namespace { static const bool _dwhbll_test_registered = \
        (::dwhbll::meta::collect_annotated<::dwhbll::test::detail::discovery_traits, ^^::, ::dwhbll::meta::fixed_string(__FILE__)>(), true); }
