// based on GCC's strify2.c
// { dg-do run }

#include <cstdlib>
#include <cstring>

#define err(str) do { abort(); } while (0)

#define str(x) #x
#define xstr(x) str(x)
#define strvar(...) #__VA_ARGS__

#define glibc_str(x) glibc_str2 (w, x)
#define glibc_str2(w, x) #x
#define ver GLIBC_2.2

#define glibc_hack(x, y) x@y

int main() {
    // Test stringification without inserting a spurious space
    char a[] = xstr(glibc_hack(foo, bar));
    if (strcmp(a, "foo@bar"))
        err("stringification without spaces");

    // Test that variable-argument stringification works
    if (strcmp(strvar(foo, bar), "foo, bar"))
        err("variable argument stringification");

    // Test that macro arguments are expanded before stringification
    // when using the two-level expansion pattern
    if (strcmp(xstr(__INCLUDE_LEVEL__), "0"))
        err("macro expansion");

    // Test that the macro name is stringified, not its value
    if (strcmp(str(__INCLUDE_LEVEL__), "__INCLUDE_LEVEL__"))
        err("macro name");

    // Test stringification of an empty argument
    if (strcmp(str(), "") || strcmp(str(), ""))
      err("empty string");

    // Test stringification of a string literal token
    if (strcmp(str("s\n"), "\"s\\n\""))
        err("quoted string");

    // Test whitespace handling inside stringification
    if (strcmp(str(a    b@ c   ), "a b@ c"))
        err("internal whitespace");

    // Test that backslash tokens do not break stringification
    if (strcmp(str(a \n), "a \n"))
        err("backslash token");

    // Test stringification through a macro chain (glibc_hack)
    if (strcmp(glibc_str(ver), "GLIBC_2.2"))
        err("whitespace");

    return 0;
}
