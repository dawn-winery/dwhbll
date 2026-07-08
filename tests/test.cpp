#include <dwhbll/testing/testing.hpp>

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
[[=skip()]]
void test_6() {}

[[=test]]
[[=tag("foo")]]
[[=skip()]]
void test_7() {}

DWHBLL_TEST_REGISTER_FILE();
