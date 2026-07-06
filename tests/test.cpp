#include <dwhbll/testing/testing.hpp>

[[=dwhbll::test::test]]
[[=dwhbll::test::name("test1")]]
void test1() {
    return;
}

[[=dwhbll::test::test]]
void test2() {
    return;
}

DWHBLL_TEST_REGISTER_FILE();
