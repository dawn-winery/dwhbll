// based on GCC's lexident.c
// { dg-do run }

#include <cstdlib>

#define err(str) do { abort(); } while (0)

// Test basic identifiers
int identifier = 1;
int _underscore = 2;
int _ = 3;
int with_underscore = 4;
int withNumber123 = 5;

// Test identifiers with escaped newlines
#def\
ine foo_ 10

#d\
ef\
ine bar 20

int main() {
    // Test that escaped newlines in identifiers work
    if (foo_ != 10)
        err("escaped newline in define");
    if (bar != 20)
        err("multiple escaped newlines in define");

    // Test identifiers starting with underscore
    int _private = 30;
    int __reserved = 40;
    if (_private != 30 || __reserved != 40)
        err("underscore identifiers");

    // Test identifiers with mixed case
    int MixedCase = 50;
    int mixedCase = 60;
    int MIXEDCASE = 70;
    if (MixedCase != 50 || mixedCase != 60 || MIXEDCASE != 70)
        err("case sensitivity");

    // Test long identifiers
    int very_long_identifier_name_that_should_work_fine = 80;
    if (very_long_identifier_name_that_should_work_fine != 80)
        err("long identifier");

    // Test identifiers with numbers (not at start)
    int var1 = 1;
    int var2 = 2;
    int variable_name_123 = 3;
    if (var1 != 1 || var2 != 2 || variable_name_123 != 3)
        err("identifiers with numbers");

    return 0;
}
