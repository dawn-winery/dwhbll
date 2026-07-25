#pragma once

#include <string_view>
#include <string>
#include <vector>

namespace dwhbll::lang::c {

enum token_type {
    none,
    eof,
    newline,
    identifier,
    string_lit,
    char_lit,
    number,
    lbrace,
    rbrace,
    lparen,
    rparen,
    lbracket,
    rbracket,
    semicolon,
    comma,
    colon,
    question,
    dot,
    arrow,
    kw_if,
    kw_else,
    kw_for,
    kw_while,
    kw_do,
    kw_switch,
    kw_case,
    kw_return,
    kw_break,
    kw_continue,
    kw_struct,
    kw_union,
    kw_enum,
    kw_typedef,
    kw_static,
    kw_extern,
    kw_const,
    kw_void,
    kw_char,
    kw_int,
    kw_short,
    kw_long,
    kw_float,
    kw_double,
    kw_signed,
    kw_unsigned,
    kw_sizeof,
    kw_auto,
    kw_register,
    kw_volatile,
    kw_goto,
    kw_default,
    operator_,
    pp_include,
    pp_define,
    pp_other,
    line_comment,
    block_comment,
};

token_type kw_type(std::string_view word);
std::string_view token_name(token_type type);

class token {
public:
    token_type type;
    std::string value;
    int line, column;

    token(token_type type, std::string value, int line, int column)
        : type(type), value(value), line(line), column(column) {}
};

std::vector<token> tokenize(std::string_view source);

} // namespace dwhbll::lang::c
