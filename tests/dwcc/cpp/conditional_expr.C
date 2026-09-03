// based on GCC's if-1.c
// { dg-do preprocess }

#if 0xa != 10
#error 0xa != 10
#endif

#if 077 != 63
#error 077 != 63
#endif

#if 0xabc != 2748
#error 0xabc != 2748
#endif

// Test arithmetic in #if
#if (1 + 2) != 3
#error arithmetic failed
#endif

#if (5 * 2) != 10
#error multiplication failed
#endif

#if (10 / 2) != 5
#error division failed
#endif

#if (10 % 3) != 1
#error modulo failed
#endif

// Test comparison operators
#if (10 > 5) && (5 < 10)
#else
#error comparison failed
#endif

#if (10 >= 10) && (5 <= 5)
#else
#error comparison failed
#endif

#if (10 == 10) && (5 != 6)
#else
#error equality failed
#endif

// Test logical operators
#if (1 && 1)
#else
#error logical and failed
#endif

#if (1 || 0)
#else
#error logical or failed
#endif

#if !0
#else
#error logical not failed
#endif

// Test bitwise operators
#if (1 | 2) == 3
#else
#error bitwise or failed
#endif

#if (3 & 1) == 1
#else
#error bitwise and failed
#endif

#if (3 ^ 1) == 2
#else
#error bitwise xor failed
#endif

#if (~1 & 2) == 2
#else
#error bitwise not failed
#endif

#if (1 << 2) == 4
#else
#error left shift failed
#endif

#if (8 >> 2) == 2
#else
#error right shift failed
#endif

// Test ternary operator
#if (1 ? 42 : 0) != 42
#error ternary failed
#endif

#if (0 ? 0 : 42) != 42
#error ternary failed
#endif

// Test defined() in expressions
#define TEST_MACRO 1
#if defined(TEST_MACRO) && TEST_MACRO == 1
#else
#error defined in expression failed
#endif

#if !defined(NONEXISTENT_MACRO)
#else
#error defined negative failed
#endif

// Test character constants in #if
#if 'A' == 65
#else
#error char constant failed
#endif

#if L'A' == 65
#else
#error wide char constant failed
#endif

// Test that integer constant expressions work
#define EXPR (10 + 20 * 3)
#if EXPR == 70
#else
#error complex expression failed
#endif
