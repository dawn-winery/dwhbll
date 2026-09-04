// based on GCC's strify1.c
// { dg-do preprocess }

#define OK1(x) #x
#define OK2(x) "prefix" #x "suffix"
#define MACRO(x) STR(x)
#define STR(x) #x
#define MAKE_STR(x) #x
#define OP_STR(x) #x

int main() {
    // Basic stringification
    OK1(hello);
    OK1(123);
    OK1(foo bar);

    // Stringification with other tokens
    OK2(test);

    // Multiple stringifications
    STR(abc) STR(def);

    // Stringification of macro arguments
    MACRO(hello world);

    // Stringification with empty argument
    STR();

    // Stringification in macros
    MAKE_STR(this is a test);

    // Test with operators
    OP_STR(a + b);
    OP_STR(x == y);

    // Test stringification preserves whitespace
    STR(  spaced  );

    return 0;
}
