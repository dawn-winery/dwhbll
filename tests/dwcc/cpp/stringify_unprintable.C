// based on GCC's strify4.c
// { dg-do run }

#include <cstdlib>
#include <cstring>

#define err(str) do { abort(); } while (0)

#define S(X) S2(X)
#define S2(X) #X
#define TAB "	" /* Note: there is a tab character here. */

int main() {
    char a[] = S(S(TAB));

    if (strcmp(a, "\"\\\"	\\\"\""))
        err("stringification caused octal");

    return 0;
}
