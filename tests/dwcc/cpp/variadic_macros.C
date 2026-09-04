// based on GCC's vararg1.c and vararg2.c
// { dg-do run }

#include <cstdlib>
#include <cstdarg>

// Test basic varargs counting
#define count(y...)  count1 ( , ##y)
#define count1(y...) count2 (y,1,0)
#define count2(_,x0,n,y...) n
#if count() != 0 || count(A) != 1
#error Incorrect vararg argument counts
#endif

// Test varargs with arguments
#define S(str, args...) "  " str "\n", ##args

// C99 style varargs
#define c99_count(...) _c99_count1(, ##__VA_ARGS__)
#define _c99_count1(...) _c99_count2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define _c99_count2(_, x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, n, ...) n

// Test __VA_ARGS__ handling
#define print(...) do { } while(0)

int main() {
    // Test counting
    if (count() != 0 || count(A) != 1)
        abort();

    // Test S macro
    const char *s = S("foo");
    if (s[0] != ' ' || s[1] != ' ' || s[2] != 'f')
        abort();

    // Test C99 style counting
    if (c99_count() != 0 || c99_count(A) != 1 || c99_count(A, B) != 2)
        abort();

    // Test empty __VA_ARGS__
    #define EMPTY_MACRO(...) __VA_ARGS__
    int x[] = { EMPTY_MACRO() };  // Should be empty
    (void)x;

    // Test with arguments
    #define PRINTF(fmt, ...) do { } while(0)
    PRINTF("test %d", 1);
    PRINTF("test");

    // Test __VA_OPT__ (C++20/C23)
    #if __cplusplus >= 202002L || defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define VA_OPT_TEST(...) __VA_OPT__(, __VA_ARGS__)
    int arr[] = { 1 VA_OPT_TEST(2) };  // Should be {1, 2}
    #endif

    return 0;
}
