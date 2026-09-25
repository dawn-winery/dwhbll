#pragma once

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
    namespace { static const bool _dwhbll_test_registered = \
        (::dwhbll::meta::collect_annotated<::dwhbll::test::detail::discovery_traits, ^^::, ::dwhbll::meta::fixed_string(__FILE__)>(), true); }
