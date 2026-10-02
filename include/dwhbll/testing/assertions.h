#pragma once

#include <dwhbll/testing/harness.h>
#include <format>
#include <functional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace dwhbll::test {

namespace detail {

class result {
public:
    void add_failure(std::string msg, std::source_location loc);
    bool passed() const { return failures_.empty(); }
    const std::vector<failure>& failures() const { return failures_; }

private:
    std::vector<failure> failures_;
};

extern result* current_result;

void report_failure(std::string message, std::source_location loc);

} // namespace detail

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

// TODO: If not std::formattable, use dbg() on it
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

#pragma GCC diagnostic pop

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

bool expect_death(std::function<void()> fn, int expected_signal_or_exit_code,
                  std::string_view msg = {},
                  std::source_location loc = std::source_location::current());

bool expect_death(std::function<void()> fn, std::string_view expected_pattern,
                  std::string_view msg = {},
                  std::source_location loc = std::source_location::current());

} // namespace dwhbll::test

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
#define EXPECT_DEATH(expr, expected) ::dwhbll::test::expect_death([&]() { expr; }, (expected), #expr)

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
#define REQUIRE_DEATH(expr, expected) \
    do { if (!::dwhbll::test::expect_death([&]() { expr; }, (expected), #expr)) return; } while (0)
