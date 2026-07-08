#pragma once

#include <dwhbll/testing/testing_detail.hpp>

#include <format>
#include <meta>
#include <source_location>
#include <string>
#include <string_view>

namespace dwhbll::test {

struct tag_filter {
    std::vector<std::string> include;
    std::vector<std::string> exclude;

    bool matches(std::vector<std::string_view> tags) const;
};

bool expect(bool cond, std::string_view msg = {},
            std::source_location loc = std::source_location::current());

template <typename A, typename B>
bool expect_eq(A const& a, B const& b,
               std::source_location loc = std::source_location::current()) {
    bool ok = (a == b);
    if (!ok) {
        if constexpr (std::formattable<A, char> && std::formattable<B, char>)
            detail::report_failure(std::format("expected {} == {}", a, b), loc);
        else
            detail::report_failure("expect_eq failed (and values are not formattable)", loc);
    }
    return ok;
}

template <typename A, typename B>
bool expect_ne(A const& a, B const& b,
               std::source_location loc = std::source_location::current()) {
    bool ok = (a != b);
    if (!ok) {
        if constexpr (std::formattable<A, char> && std::formattable<B, char>)
            detail::report_failure(std::format("expected {} != {}", a, b), loc);
        else
            detail::report_failure("expect_ne failed (and values are not formattable)", loc);
    }
    return ok;
}

tag_filter parse_filter(std::string_view expr);

int run_all();
int run_all(tag_filter const& filter);
int run_all(int argc, char** argv);

} // namespace dwhbll::test

#define DWHBLL_ASSERT(cond) \
    do { if (!::dwhbll::test::expect((cond), #cond)) return; } while (0)
#define DWHBLL_ASSERT_EQ(a, b) \
    do { if (!::dwhbll::test::expect_eq((a), (b))) return; } while (0)
#define DWHBLL_ASSERT_NE(a, b) \
    do { if (!::dwhbll::test::expect_ne((a), (b))) return; } while (0)

#define DWHBLL_TEST_REGISTER_FILE() \
    namespace { static const bool _ = \
        (::dwhbll::test::detail::collect_tests<^^::, ::dwhbll::test::detail::fixed_string(__FILE__)>(), true); }
