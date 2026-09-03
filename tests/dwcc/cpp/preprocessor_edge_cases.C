// { dg-do run }

#include <cstdlib>

#define err(str) do { abort(); } while (0)

#define f(a,b) f2(a,,b)
#define f2(a,b,c) a; b; c;
#define f3(a) a

#define g() p()

void p(void) {}

int main() {
    f(p(), p());
    f2(p(), , p());
    f3();
    g();

    #define SAME 1
    #define SAME 1
    #define SAME2(x) x
    #define SAME2(x) x

    #define NO_ARGS 42
    #define WITH_EMPTY() 42
    if (NO_ARGS != 42)
        err("no args macro");
    if (WITH_EMPTY() != 42)
        err("empty args macro");

    #define EXPANDS_TO_NOTHING
    int y = 42 EXPANDS_TO_NOTHING;  // int y = 42;

    #define TRIPLE_PASTE(a, b, c) a ## b ## c
    int TRIPLE_PASTE(a, b, c) = 20;  // int abc = 20;

    #define VARIADIC(...) __VA_ARGS__
    int arr1[] = { VARIADIC(1, 2, 3) };  // { 1, 2, 3 }

    return 0;
}
