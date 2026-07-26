#include <functional>
#include <optional>
#include <dwhbll/lang/c/tokenize.h>

using namespace dwhbll::lang::c;

bool test_tokenizer() {
    std::string code = "#include <stdio.h>\n"
        "int main() {\n"
        "   printf(\"Hello World\");\n"
        "}\n";

    std::vector<token> tokens = tokenize(code);
    for (token t : tokens)
        std::printf("%s, %d:%d\n", token_name(t.type).data(), t.line, t.column);

#define TEST(x, y) if (x != y) { \
        std::printf("failed: %d != %d\n", x, y); \
        return false;\
    }

    TEST((int)tokens.size(), 17)

#define IS_TYPE(x, y) TEST(tokens[x].type, y)

    IS_TYPE(0, pp_include)
    IS_TYPE(1, newline)
    IS_TYPE(2, kw_int)
    IS_TYPE(3, identifier)
    IS_TYPE(4, lparen)
    IS_TYPE(5, rparen)
    IS_TYPE(6, lbrace)
    IS_TYPE(7, newline)
    IS_TYPE(8, identifier)
    IS_TYPE(9, lparen)
    IS_TYPE(10, string_lit)
    IS_TYPE(11, rparen)
    IS_TYPE(12, semicolon)
    IS_TYPE(13, newline)
    IS_TYPE(14, rbrace)
    IS_TYPE(15, newline)
    IS_TYPE(16, eof)

#undef IS_TYPE
#undef TEST

    return true;
}

std::unordered_map<std::string, std::function<bool()>> c_lang_dispatch = {
    {"tokenizer", test_tokenizer},
};

bool c_lang_test(std::optional<std::string> test_to_run) {
  if (test_to_run.has_value() && c_lang_dispatch.contains(test_to_run.value()))
    return c_lang_dispatch.at(test_to_run.value())();
  return 1;
}
