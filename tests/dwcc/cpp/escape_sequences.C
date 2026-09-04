// based on GCC's escape-1.c and lexstrng.c
// { dg-do run }

#include <cstdlib>
#include <cstring>
#include <cwchar>

#define err(str) do { abort(); } while (0)

int main() {
    // Basic escape sequences
    const char *s1 = "hello\nworld";
    if (s1[5] != '\n')
        err("newline escape");

    const char *s2 = "tab\there";
    if (s2[3] != '\t')
        err("tab escape");

    // Octal escapes
    const char *s3 = "\101\102\103";  // ABC
    if (strcmp(s3, "ABC") != 0)
        err("octal escape");

    // Hex escapes
    const char *s4 = "\x41\x42\x43";  // ABC
    if (strcmp(s4, "ABC") != 0)
        err("hex escape");

    // Escaped newline in string literal
    const char *s5 = "line1\
line2";
    if (strcmp(s5, "line1line2") != 0)
        err("escaped newline");

    // Escaped newline in char literal
    const char c1 = 'a\
b';
    if (c1 != 'b')
        err("escaped newline in char");

    // Escaped terminators
    const char *term = "\"\\\"\\";
    if (term[0] != '"' || term[1] != '\\' || term[2] != '"' || term[3] != '\\' || term[4] != '\0')
        err("escaped string terminators");

    const char termc = '\'';
    const char *terms = "'";
    if (termc != terms[0])
        err("escaped char terminator");

    // Wide strings and chars
    const wchar_t wchar = L'w';
    const wchar_t *wstring = L"wide string";
    if (wchar != L'w')
        err("wide char");
    if (wcscmp(wstring, L"wide string") != 0)
        err("wide string");

    // Universal character names
    const char *ucn1 = "\u0041";  // A
    const char *ucn2 = "\U00000041";  // A
    if (ucn1[0] != 'A' || ucn2[0] != 'A')
        err("UCN in string");

    return 0;
}
