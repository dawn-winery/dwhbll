// #line, #error, #pragma, conditionals
// { dg-do run }

#include <cstdlib>

#define err(str) do { abort(); } while (0)

// Test #error
#if 0
#error This should not trigger
#endif

// Test #pragma
#pragma GCC poison unused_pragma_test
#pragma message "This is a test message"

// Test #if, #elif, #else, #endif
#if 1
#define COND_TEST 1
#elif 0
#define COND_TEST 2
#else
#define COND_TEST 3
#endif

#if defined(COND_TEST) && COND_TEST == 1
#define COND_TEST_PASSED 1
#endif

// Test #ifdef, #ifndef
#define DEFINED_MACRO 1
#ifdef DEFINED_MACRO
#define IFDEF_TEST 1
#endif

#ifndef UNDEFINED_MACRO
#define IFNDEF_TEST 1
#endif

// Test #defined operator
#if defined(DEFINED_MACRO) && !defined(UNDEFINED_MACRO)
#define DEFINED_OP_TEST 1
#endif

// Test nested conditionals
#if 1
#if 1
#define NESTED_TEST 1
#endif
#endif

// Test #elif with multiple conditions
#if 0
#define ELIF_TEST 1
#elif 0
#define ELIF_TEST 2
#elif 1
#define ELIF_TEST 3
#else
#define ELIF_TEST 4
#endif

// Test __FILE__, __LINE__, __DATE__, __TIME__, __STDC__, __STDC_VERSION__
const char *file_test = __FILE__;
int line_test = __LINE__;
const char *date_test = __DATE__;
const char *time_test = __TIME__;

#if defined(__STDC__)
#define STDC_TEST 1
#endif

#if defined(__STDC_VERSION__)
#define STDC_VERSION_TEST __STDC_VERSION__
#endif

// Test #undef
#define TO_UNDEF 1
#undef TO_UNDEF
#ifndef TO_UNDEF
#define UNDEF_TEST 1
#endif

// Test macro same-value redefinition
#define REDEF 1
#define REDEF 1

// Test empty macro
#define EMPTY_MACRO
#define EMPTY_MACRO2()

// Test macro with no replacement list
#define NO_REPLACEMENT
#if defined(NO_REPLACEMENT)
#define NO_REPLACEMENT_TEST 1
#endif

int main() {
    #ifndef COND_TEST_PASSED
        err("#if/#elif/#else");
    #endif

    #ifndef IFDEF_TEST
        err("#ifdef");
    #endif

    #ifndef IFNDEF_TEST
        err("#ifndef");
    #endif

    #ifndef DEFINED_OP_TEST
        err("#defined operator");
    #endif

    #ifndef NESTED_TEST
        err("nested conditionals");
    #endif

    if (ELIF_TEST != 3)
        err("#elif chain");

    #ifndef UNDEF_TEST
        err("#undef");
    #endif

    #ifndef NO_REPLACEMENT_TEST
        err("empty macro definition");
    #endif

    return 0;
}
