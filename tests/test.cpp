#include <dwhbll/testing/testing.h>

using namespace dwhbll::test;

[[=test]]
void test_1() {}

[[=test]]
[[=tag("foo")]]
void test_2() {}

[[=test]]
[[=tag("bar")]]
void test_3() {}

[[=test]]
[[=tag("foo")]]
[[=tag("bar")]]
void test_4() {}

[[=test]]
[[=tag("baz")]]
void test_5() {}

[[=test]]
[[=skip("some_reason_1")]]
void test_6() {}

[[=test]]
[[=tag("foo")]]
[[=skip("some_reason_2")]]
void test_7() {}

[[=test]]
[[=name("name")]]
void test_8() {}

[[=test]]
[[=tag("known_issue")]]
[[=xfail("expected failure")]]
void test_9() {
    REQUIRE(false);
}

[[=test]]
[[=name("assertions_and_expectations")]]
void test_10() {
    EXPECT(1 + 1 == 2);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
    EXPECT_EQ(42, 42);
    EXPECT_NE(42, 100);
    EXPECT_LT(5, 10);
    EXPECT_LE(5, 5);
    EXPECT_GT(10, 5);
    EXPECT_GE(10, 10);

    int val = 123;
    int* ptr = &val;
    int* null_ptr = nullptr;
    EXPECT_NOT_NULL(ptr);
    EXPECT_NULL(null_ptr);

    EXPECT_THROWS(std::runtime_error, throw std::runtime_error("expected"));
    EXPECT_NO_THROW(val += 1);

    REQUIRE_EQ(val, 124);
    REQUIRE_NE(val, 0);
    REQUIRE_TRUE(val > 0);
}

TEST_REGISTER_FILE();
