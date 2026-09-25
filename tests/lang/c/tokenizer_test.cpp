#include <dwhbll/macros/testing.h>

import dwhbll.testing;
import dwhbll.lang;
import std;

using namespace dwhbll::test;
using namespace dwhbll::lang::c;

namespace lang::c {

[[=test]]
void tokenizer()
{
    std::string code = "#include <stdio.h>\n"
        "int main() {\n"
        "   printf(\"Hello World\");\n"
        "}\n";

    std::vector<token> tokens = tokenize(code);
    for (token t : tokens)
        std::printf("%s, %d:%d\n", token_name(t.type).data(), t.line, t.column);

    REQUIRE_EQ((int)tokens.size(), 17);

    EXPECT_EQ(tokens[0].type, pp_include);
    EXPECT_EQ(tokens[1].type, newline);
    EXPECT_EQ(tokens[2].type, kw_int);
    EXPECT_EQ(tokens[3].type, identifier);
    EXPECT_EQ(tokens[4].type, lparen);
    EXPECT_EQ(tokens[5].type, rparen);
    EXPECT_EQ(tokens[6].type, lbrace);
    EXPECT_EQ(tokens[7].type, newline);
    EXPECT_EQ(tokens[8].type, identifier);
    EXPECT_EQ(tokens[9].type, lparen);
    EXPECT_EQ(tokens[10].type, string_lit);
    EXPECT_EQ(tokens[11].type, rparen);
    EXPECT_EQ(tokens[12].type, semicolon);
    EXPECT_EQ(tokens[13].type, newline);
    EXPECT_EQ(tokens[14].type, rbrace);
    EXPECT_EQ(tokens[15].type, newline);
    EXPECT_EQ(tokens[16].type, eof);
}

}

TEST_REGISTER_FILE();
