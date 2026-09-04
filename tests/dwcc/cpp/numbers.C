// based on GCC's lexnum.c
// { dg-do run }

#include <cstdlib>
#include <cmath>

#define err(str) do { abort(); } while (0)

int main() {
    // Decimal integers
    int i1 = 42;
    int i2 = 0;
    int i3 = 123456789;
    if (i1 != 42 || i2 != 0 || i3 != 123456789)
        err("decimal ints");

    // Octal integers
    int o1 = 077;  // 63
    int o2 = 0;
    if (o1 != 63 || o2 != 0)
        err("octal ints");

    // Hex integers
    int h1 = 0xFF;  // 255
    int h2 = 0xabc;  // 2748
    int h3 = 0XABC;  // 2748
    if (h1 != 255 || h2 != 2748 || h3 != 2748)
        err("hex ints");

    // Integer suffixes
    unsigned u1 = 42u;
    unsigned u2 = 42U;
    long l1 = 42l;
    long l2 = 42L;
    unsigned long ul1 = 42ul;
    unsigned long ul2 = 42UL;
    unsigned long ul3 = 42lu;
    unsigned long ul4 = 42LU;
    long long ll1 = 42ll;
    long long ll2 = 42LL;
    unsigned long long ull1 = 42ull;
    unsigned long long ull2 = 42ULL;

    // Floating point literals
    float f1 = 1.0f;
    float f2 = 1.F;
    double d1 = 1.0;
    double d2 = 1.;
    double d3 = .1;
    long double ld1 = 1.0L;
    long double ld2 = 1.L;

    // Exponential notation
    double e1 = 1e10;
    double e2 = 1E10;
    double e3 = 1e-10;
    double e4 = 1E-10;
    double e5 = 1.5e2;  // 150
    double e6 = .5e1;   // 5
    double e7 = 5.e1;   // 50

    // Hex floating point (C99/C++17)
    double hf1 = 0x1p0;   // 1.0
    double hf2 = 0x1p1;   // 2.0
    double hf3 = 0x1p-1;  // 0.5
    double hf4 = 0xAp0;   // 10.0
    double hf5 = 0x1.8p1; // 3.0

    // Test escaped newlines in numbers
    #define num 12\
34
    if (num != 1234)
        err("escaped newline in number");

    // Test all suffixes combinations
    float f3 = 1.0f;
    double d4 = 1.0;
    long double ld3 = 1.0L;

    return 0;
}
