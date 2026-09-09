// based on GCC's raw-string-directive-2.c
// { dg-do run }

#include <cstdlib>
#include <cstring>

#define err(str) do { abort(); } while (0)

// Multline raw string defined via a macro
#define S1 R"(three
line
string)"

// Raw string concatenated with a regular string at compile time
#define S2 R"(pasted
two line)" " string"

// Macro that accepts a raw string and appends another at expansion
#define X(a, b) a b R"(
one more)"

const char *s1 = S1;
const char *s2 = S2;
const char *s3 = X(S1, R"(
with this line plus)");

int main() {
    const char s1_correct[] = "three\nline\nstring";
    if (strcmp(s1, s1_correct) != 0)
        err("multiline raw string in macro");

    const char s2_correct[] = "pasted\ntwo line string";
    if (strcmp(s2, s2_correct) != 0)
        err("raw string concatenation");

    const char s3_correct[] = "three\nline\nstring\nwith this line plus\none more";
    if (strcmp(s3, s3_correct) != 0)
        err("macro expansion with raw strings");

    return 0;
}
