#include <dwhbll/testing/testing.hpp>

using namespace dwhbll::test;

[[=test]]
[[=skip("reason")]]
[[=name("test1")]]
void test1() {
    return;
}

[[=test]]
void test2() {
    return;
}

DWHBLL_TEST_REGISTER_FILE();
