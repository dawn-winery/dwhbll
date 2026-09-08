#pragma once

#include <dwhbll/testing/testing_detail.h>
#include <dwhbll/testing/harness.h>
#include <dwhbll/testing/runner.h>

#include <format>
#include <meta>
#include <source_location>
#include <string>
#include <string_view>

namespace dwhbll::test {

bool expect(bool cond, std::string_view msg = {},
            std::source_location loc = std::source_location::current());

inline bool expect_true(bool cond, std::string_view msg = "expected true",
                        std::source_location loc = std::source_location::current()) {
    return expect(cond, msg, loc);
}

inline bool expect_false(bool cond, std::string_view msg = "expected false",
                         std::source_location loc = std::source_location::current()) {
    return expect(!cond, msg, loc);
}

// TODO: dbg() the types if not std::formattable
template <typename T>
bool expect_null(const T* ptr, std::source_location loc = std::source_location::current()) {
    bool ok = (ptr == nullptr);
    if (!ok)
        detail::report_failure(std::format("expected null pointer, got {}", static_cast<const void*>(ptr)), loc);
    return ok;
}

template <typename T>
bool expect_not_null(const T* ptr, std::source_location loc = std::source_location::current()) {
    bool ok = (ptr != nullptr);
    if (!ok)
        detail::report_failure("expected non-null pointer, got nullptr", loc);
    return ok;
}

template <typename A, typename B>
bool expect_eq(const A& a, const B& b,
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
bool expect_ne(const A& a, const B& b,
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

template <typename A, typename B>
bool expect_lt(const A& a, const B& b,
               std::source_location loc = std::source_location::current()) {
    bool ok = (a < b);
    if (!ok) {
        if constexpr (std::formattable<A, char> && std::formattable<B, char>)
            detail::report_failure(std::format("expected {} < {}", a, b), loc);
        else
            detail::report_failure("expect_lt failed", loc);
    }
    return ok;
}

template <typename A, typename B>
bool expect_le(const A& a, const B& b,
               std::source_location loc = std::source_location::current()) {
    bool ok = (a <= b);
    if (!ok) {
        if constexpr (std::formattable<A, char> && std::formattable<B, char>)
            detail::report_failure(std::format("expected {} <= {}", a, b), loc);
        else
            detail::report_failure("expect_le failed", loc);
    }
    return ok;
}

template <typename A, typename B>
bool expect_gt(const A& a, const B& b,
               std::source_location loc = std::source_location::current()) {
    bool ok = (a > b);
    if (!ok) {
        if constexpr (std::formattable<A, char> && std::formattable<B, char>)
            detail::report_failure(std::format("expected {} > {}", a, b), loc);
        else
            detail::report_failure("expect_gt failed", loc);
    }
    return ok;
}

template <typename A, typename B>
bool expect_ge(const A& a, const B& b,
               std::source_location loc = std::source_location::current()) {
    bool ok = (a >= b);
    if (!ok) {
        if constexpr (std::formattable<A, char> && std::formattable<B, char>)
            detail::report_failure(std::format("expected {} >= {}", a, b), loc);
        else
            detail::report_failure("expect_ge failed", loc);
    }
    return ok;
}

template <typename Exception, typename Fn>
bool expect_throws(Fn&& fn, std::source_location loc = std::source_location::current()) {
    try {
        fn();
    } catch (const Exception&) {
        return true;
    } catch (const std::exception& e) {
        detail::report_failure(std::format("expected exception of specific type, but caught: {}", e.what()), loc);
        return false;
    } catch (...) {
        detail::report_failure("expected exception of specific type, but caught unknown exception", loc);
        return false;
    }
    detail::report_failure("expected exception to be thrown, but nothing was thrown", loc);
    return false;
}

template <typename Fn>
bool expect_no_throw(Fn&& fn, std::source_location loc = std::source_location::current()) {
    try {
        fn();
        return true;
    } catch (const std::exception& e) {
        detail::report_failure(std::format("expected no exception, but caught: {}", e.what()), loc);
        return false;
    } catch (...) {
        detail::report_failure("expected no exception, but caught unknown exception", loc);
        return false;
    }
}

int run_all(const options& options = {});
int run_all(const tag_filter& filter);

} // namespace dwhbll::test

// Non-fatal expectation macros
#define EXPECT(cond) ::dwhbll::test::expect((cond), #cond)
#define EXPECT_TRUE(cond) ::dwhbll::test::expect_true((cond), #cond)
#define EXPECT_FALSE(cond) ::dwhbll::test::expect_false((cond), "!(" #cond ")")
#define EXPECT_EQ(a, b) ::dwhbll::test::expect_eq((a), (b))
#define EXPECT_NE(a, b) ::dwhbll::test::expect_ne((a), (b))
#define EXPECT_LT(a, b) ::dwhbll::test::expect_lt((a), (b))
#define EXPECT_LE(a, b) ::dwhbll::test::expect_le((a), (b))
#define EXPECT_GT(a, b) ::dwhbll::test::expect_gt((a), (b))
#define EXPECT_GE(a, b) ::dwhbll::test::expect_ge((a), (b))
#define EXPECT_NULL(p) ::dwhbll::test::expect_null((p))
#define EXPECT_NOT_NULL(p) ::dwhbll::test::expect_not_null((p))
#define EXPECT_THROWS(E, ...) ::dwhbll::test::expect_throws<E>([&]() { __VA_ARGS__; })
#define EXPECT_NO_THROW(...) ::dwhbll::test::expect_no_throw([&]() { __VA_ARGS__; })

// Fatal requirement macros (returns on failure)
#define REQUIRE(cond) \
    do { if (!::dwhbll::test::expect((cond), #cond)) return; } while (0)
#define REQUIRE_TRUE(cond) \
    do { if (!::dwhbll::test::expect_true((cond), #cond)) return; } while (0)
#define REQUIRE_FALSE(cond) \
    do { if (!::dwhbll::test::expect_false((cond), "!(" #cond ")")) return; } while (0)
#define REQUIRE_EQ(a, b) \
    do { if (!::dwhbll::test::expect_eq((a), (b))) return; } while (0)
#define REQUIRE_NE(a, b) \
    do { if (!::dwhbll::test::expect_ne((a), (b))) return; } while (0)
#define REQUIRE_LT(a, b) \
    do { if (!::dwhbll::test::expect_lt((a), (b))) return; } while (0)
#define REQUIRE_LE(a, b) \
    do { if (!::dwhbll::test::expect_le((a), (b))) return; } while (0)
#define REQUIRE_GT(a, b) \
    do { if (!::dwhbll::test::expect_gt((a), (b))) return; } while (0)
#define REQUIRE_GE(a, b) \
    do { if (!::dwhbll::test::expect_ge((a), (b))) return; } while (0)
#define REQUIRE_NULL(p) \
    do { if (!::dwhbll::test::expect_null((p))) return; } while (0)
#define REQUIRE_NOT_NULL(p) \
    do { if (!::dwhbll::test::expect_not_null((p))) return; } while (0)
#define REQUIRE_THROWS(E, ...) \
    do { if (!::dwhbll::test::expect_throws<E>([&]() { __VA_ARGS__; })) return; } while (0)
#define REQUIRE_NO_THROW(...) \
    do { if (!::dwhbll::test::expect_no_throw([&]() { __VA_ARGS__; })) return; } while (0)

#define TEST_REGISTER_FILE() \
    namespace { static const bool _ = \
        (::dwhbll::test::detail::collect_tests<^^::, ::dwhbll::test::detail::fixed_string(__FILE__)>(), true); }
