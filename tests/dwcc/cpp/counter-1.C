/* Source: GCC */
/* { dg-do preprocess } */
/* Tests __COUNTER__ macro is correctly expanded.  */

#define counter __COUNTER__

#if __COUNTER__ != 0
#error __COUNTER__ != 0
#endif

#if counter != 1
#error counter != 1
#endif

#if __COUNTER__ != 2
#error __COUNTER__ != 2
#endif
