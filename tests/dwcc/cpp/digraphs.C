// based on GCC's digraphs.c
// { dg-do run }

#include <cstdlib>
#include <cstring>

#define err(str) do { abort(); } while (0)

// Test digraphs in array subscripts
int arr<:10:>;  // int arr[10];

// Test digraphs in blocks
int main() <%  // {
    // Test digraph operators
    %:define glue(x, y) x %:%: y  // #define glue(x, y) x ## y

    // Test digraph stringification
    %:define str(x) %:x  // #define str(x) #x

    // Test digraphs in array
    const char di_str<::> = str(%:%:<::><%%>%:);  // const char di_str[] = "#<:><%>#";

    // Check glue macro works
    if (strcmp(di_str, "%:%:<::><%%>%:") != 0)
        err("Digraph spelling not preserved!");

    // Test digraphs as operators
    int a<:5:> = {1, 2, 3, 4, 5};  // int a[5] = {...};
    if (a<:0:> != 1 || a<:4:> != 5)
        err("Digraph array access");

    // Test digraph block
    if (1) <%  // {
        int x = 42;
        if (x != 42)
            err("Digraph block");
    %>  // }

    // Test alternative tokens
    // and, or, not, bitand, bitor, xor, compl
    if (1 and 1) { }  // &&
    if (0 or 1) { }   // ||
    if (not 0) { }    // !
    int x = 1 bitand 2;  // &
    int y = 1 bitor 2;   // |
    int z = 1 xor 2;     // ^
    int w = compl 1;     // ~

    return 0;
%>  // }
