// based on GCC's raw-string-1.c
// { dg-do run }

#include <cstdlib>
#include <cstring>

#define err(str) do { abort(); } while (0)

// Raw strings preserve backslashes
const char s0[] = R"(a\
\u010d\U0000010D\\\'\"\?\a\b\f\n\r\t\v\0\00\000\xa\xabb
c)";
const char s1[] = "a\\\n\\u010d\\U0000010D\\\\\\'\\\"\\?\\a\\b\\f\\n\\r\\t\\v\\0\\00\\000\\xa\\xabb\nc";

// Raw strings with a custom delimiter set
const char s2[] = R"*|*(a\
b
c)"
c)*|"
c)*|*";
const char s3[] = "a\\\nb\nc)\"\nc)*|\"\nc";

// Trigraphs inside raw strings are not expanded
const char s4[] = R"(??/
??/
??(??<??=??'??!??-??>??)";
const char s5[] = "?\?/\n?\?/\n?\?(?\?<?\?=?\?'?\?!?\?-?\?>?\?";

// Empty raw string
const char s6[] = R"()";
const char s7[] = "";

int main() {
    if (sizeof(s0) != sizeof(s1) || memcmp(s0, s1, sizeof(s0)) != 0)
        err("raw string with escapes");

    if (sizeof(s2) != sizeof(s3) || memcmp(s2, s3, sizeof(s2)) != 0)
        err("raw string with custom delimiter");

    if (sizeof(s4) != sizeof(s5) || memcmp(s4, s5, sizeof(s4)) != 0)
        err("trigraphs in raw string");

    if (sizeof(s6) != sizeof(s7) || memcmp(s6, s7, sizeof(s6)) != 0)
        err("empty raw string");

    return 0;
}
