#pragma once

#include <version>

#if __cpp_impl_reflection >= 202506L

#include <dwhbll/testing/testing_detail.hpp>

#include <cstddef>
#include <format>
#include <meta>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace dwhbll::test {

template <std::size_t N>
struct TestInfo {
    char name[N]{};
    bool skip = false;
};

template <std::size_t N>
consteval auto Test(char const (&s)[N], bool skip = false) {
    TestInfo<N> t{};
    for (std::size_t i = 0; i < N; ++i) t.name[i] = s[i];
    t.skip = skip;
    return t;
}

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

int run_all();

} // namespace dwhbll::test

#define DWHBLL_ASSERT(cond) \
    do { if (!::dwhbll::test::expect((cond), #cond)) return; } while (0)
#define DWHBLL_ASSERT_EQ(a, b) \
    do { if (!::dwhbll::test::expect_eq((a), (b))) return; } while (0)
#define DWHBLL_ASSERT_NE(a, b) \
    do { if (!::dwhbll::test::expect_ne((a), (b))) return; } while (0)

#define DWHBLL_TEST_REGISTER_FILE() \
    namespace { static const bool registered_ = ::dwhbll::test::detail::register_file(); }

#endif
