// based on GCC's _Pragma1.c
// { dg-do run }

#include <cstdlib>

#define err(str) do { abort(); } while (0)

// Standard use of _Pragma
_Pragma("message \"Test pragma message\"")

// Test _Pragma in macro
#define PRAGMA_MSG(msg) _Pragma(#msg)

PRAGMA_MSG(message "Pragma from macro")

// Test _Pragma with stringification
#define STRINGIFY(x) #x
#define PRAGMA_STRING(x) _Pragma(STRINGIFY(x))

PRAGMA_STRING(message "Pragma with stringify")

// Test _Pragma with token pasting
#define PASTE(a, b) a ## b
#define PRAGMA_CONCAT(a, b) _Pragma(PASTE(a, b))

_Pragma("_Pragma(\"message \\\"nested\\\"\") message \"outer\"")

// Test _Pragma in false conditional
#if 0
_Pragma("message \"This should not appear\"")
#endif

// Test that _Pragma is not recognized in #if expressions
#if 1 _Pragma("message \"in if\"")
// The above line tests that _Pragma is not interpreted in #if
#endif

int main() {
    return 0;
}
