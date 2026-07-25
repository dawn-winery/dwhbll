#include <cctype>
#include <string>

#include "dwhbll/lang/c/tokenize.h"

namespace dwhbll::lang::c {

constexpr unsigned int hash(std::string word, int h = 0) {
    return !word[h] ? 5381 : (hash(word, h + 1) * 33) ^ word[h];
}

token_type kw_type(std::string_view word) {
    switch (hash(std::string(word))) {
    case hash("if"): return kw_if;
    case hash("else"): return kw_else;
    case hash("for"): return kw_for;
    case hash("while"): return kw_while;
    case hash("do"): return kw_do;
    case hash("switch"): return kw_switch;
    case hash("case"): return kw_case;
    case hash("return"):return kw_return;
    case hash("break"): return kw_break;
    case hash("continue"): return kw_continue;
    case hash("struct"): return kw_struct;
    case hash("union"): return kw_union;
    case hash("enum"): return kw_enum;
    case hash("typedef"): return kw_typedef;
    case hash("static"): return kw_static;
    case hash("extern"): return kw_extern;
    case hash("const"): return kw_const;
    case hash("void"): return kw_void;
    case hash("char"): return kw_char;
    case hash("int"): return kw_int;
    case hash("short"): return kw_short;
    case hash("long"): return kw_long;
    case hash("float"): return kw_float;
    case hash("double"): return kw_double;
    case hash("signed"): return kw_signed;
    case hash("unsigned"): return kw_unsigned;
    case hash("sizeof"): return kw_sizeof;
    case hash("auto"): return kw_auto;
    case hash("register"): return kw_register;
    case hash("volatile"): return kw_volatile;
    case hash("goto"): return kw_goto;
    case hash("default"): return kw_default;
    default:
        return none;
    }
}

std::string_view token_name(token_type type) {
 switch (type) {
    case none: return "";
    case eof: return "eof";
    case newline: return "newline";
    case identifier: return "identifier";
    case string_lit: return "string_lit";
    case char_lit: return "char_lit";
    case number: return "number";
    case lbrace: return "lbrace";
    case rbrace: return "rbrace";
    case lparen: return "lparen";
    case rparen: return "rparen";
    case lbracket: return "lbracket";
    case rbracket: return "rbracket";
    case semicolon: return "semicolon";
    case comma: return "comma";
    case colon: return "colon";
    case question: return "question";
    case dot: return "dot";
    case arrow: return "arrow";
    case kw_if: return "if";
    case kw_else: return "else";
    case kw_for: return "for";
    case kw_while: return "while";
    case kw_do: return "do";
    case kw_switch: return "switch";
    case kw_case: return "case";
    case kw_return: return "return";
    case kw_break: return "break";
    case kw_continue: return "continue";
    case kw_struct: return "struct";
    case kw_union: return "union";
    case kw_enum: return "enum";
    case kw_typedef: return "typedef";
    case kw_static: return "static";
    case kw_extern: return "extern";
    case kw_const: return "const";
    case kw_void: return "void";
    case kw_char: return "char";
    case kw_int: return "int";
    case kw_short: return "short";
    case kw_long: return "long";
    case kw_float: return "float";
    case kw_double: return "double";
    case kw_signed: return "signed";
    case kw_unsigned: return "unsigned";
    case kw_sizeof: return "sizeof";
    case kw_auto: return "auto";
    case kw_register: return "register";
    case kw_volatile: return "volatile";
    case kw_goto: return "goto";
    case kw_default: return "default";
    case operator_: return "operator";
    case pp_include: return "include";
    case pp_define: return "define";
    case pp_other: return "other";
    case line_comment: return "line_comment";
    case block_comment: return "block_comment";
    }
    return "unknown";
}

bool is_ident_start(char c) {
    return isalpha(c) || c == '_';
}

bool is_ident_continue(char c) {
    return is_ident_start(c) || isdigit(c);
}

std::vector<token> tokenize(std::string_view source) {
    std::vector<token> tokens;
    int pos = 0, line = 1, col = 1;
    const int n = source.size();

    while (pos < n) {
        char c = source[pos];

        if (c == '\n') {
            tokens.push_back(token(newline, "\n", line, col));
            pos++, line++, col = 1;
            continue;
        }

        if (c == '\r' || isspace(c)) {
            pos++, col++;
            continue;
        }

        if (c == '/' && pos + 1 < n) {
            if (source[pos + 1] == '/') {
                const int start = pos;
                pos += 2, col += 2;
                for (; pos < n && source[pos] != '\n'; pos++, col++)
                    ;
                const int pms = pos - start;
                tokens.push_back(token(line_comment, std::string(source.substr(start, pms)), line, col - pms));
                continue;
            }
            if (source[pos + 1] == '*') {
                const int start = pos;
                pos += 2, col += 2;
                for (; pos < n; pos++, col++) {
                    if (source[pos] == '\n') {
                        line++, pos++, col = 1;
                        continue;
                    }
                    if (source[pos] == '*' && pos + 1 < n && source[pos + 1] == '/') {
                       pos += 2, col += 2;
                       break;
                    }
                }
                const int pms = pos - start;
                tokens.push_back(token(block_comment, std::string(source.substr(start, pms)), line, col - pms));
                continue;
            }
        }

        if (c == '"' || c == '\'') {
            const int start = pos;
            const char quote = c;
            pos++, col++;
            for (; pos < n; pos++, col++) {
                if (source[pos] == '\\') {
                    pos += 2, col += 2;
                    continue;
                }
                if (source[pos] == quote) {
                    pos++, col++;
                    break;
                }
                if (source[pos] == '\n') {
                    line++, col = 1;
                }
            }
            const int pms = pos - start;
            tokens.push_back(token(quote == '"' ? string_lit : char_lit, std::string(source.substr(start, pms)), line, col - pms));
            continue;
        }

        if (isdigit(c) || (c == '.' && pos + 1 < n && isdigit(source[pos + 1]))) {
            const int start = pos;
            pos++, col++;
            if (c == '0' && pos < n && (source[pos] == 'x' || source[pos] == 'X')) {
                pos++, col++;
                for (; pos < n && isxdigit(source[pos]); pos++, col++)
                    ;
                for (; pos < n && (source[pos] == 'u' || source[pos] == 'U' || source[pos] == 'l' || source[pos] == 'L'); pos++, col++)
                    ;
            } else {
                for (; pos < n && isdigit(source[pos]); pos++, col++)
                    ;
                if (pos < n && source[pos] == '.') {
                    pos++, col++;
                    for (; pos < n && isdigit(source[pos]); pos++, col++)
                        ;
                }
                if (pos < n && (source[pos] == 'e' || source[pos] == 'E')) {
                    pos++, col++;
                    if (pos < n && (source[pos] == '+' || source[pos] == '-'))
                        pos++, col++;
                    for (; pos < n && isdigit(source[pos]); pos++, col++)
                        ;
                }
                for (; pos < n && ((source[pos] == 'u' || source[pos] == 'U') || source[pos] == 'l' || source[pos] == 'L' || source[pos] == 'f' || source[pos] == 'F'); pos++, col++)
                    ;
            }
            const int pms = pos - start;
            tokens.push_back(token(number, std::string(source.substr(start, pms)), line, col - pms));
            continue;
        }

        if (is_ident_start(c)) {
            const int start = pos;
            pos++, col++;
            for (; pos < n && is_ident_continue(source[pos]); pos++, col++)
                ;
            std::string_view word = source.substr(start, pos - start);
            token_type typ = kw_type(word);
            if (typ == none) typ = identifier;
            const int pms = pos - start;
            tokens.push_back(token(typ, std::string(word), line, col - pms));
            continue;
        }

        if (c == '#') {
            const int start = pos;
            pos++, col++;
            for (; pos < n && isspace(source[pos]); pos++, col++)
                ;
            if (pos < n && is_ident_start(source[pos])) {
                const int dir_start = pos;
                for (; pos < n && is_ident_continue(source[pos]); pos++, col++)
                    ;
                const std::string_view dir = source.substr(dir_start, pos - dir_start);
                while (pos < n && source[pos] != '\n') {
                    if (source[pos] == '\\' && pos + 1 < n && source[pos + 1] == '\n') {
                        pos += 2, line++;
                        col = 1;
                    } else pos++, col++;
                }
                const int pms = pos - start;
                const std::string_view line_text = source.substr(start, pms);
                token_type typ = pp_other;
                if (dir == "include")
                    typ = pp_include;
                else if (dir == "define")
                    typ = pp_define;
                tokens.push_back(token(typ, std::string(line_text), line, col - pms));
            } else {
                tokens.push_back(token(operator_, "#", line, col - 1));
                pos++;
            }
            continue;
        }

        if (pos + 1 < n) {
            const std::string two = std::string(source.substr(pos, 2));
            switch (hash(two)) {
            case hash("->"):
                tokens.push_back(token(arrow, "->", line, col));
                pos += 2, col += 2;
                continue;
            case hash("=="):
            case hash("!="):
            case hash("<="):
            case hash(">="):
            case hash("&&"):
            case hash("||"):
            case hash("++"):
            case hash("--"):
            case hash("+="):
            case hash("-="):
            case hash("*="):
            case hash("/="):
            case hash("%="):
            case hash("&="):
            case hash("|="):
            case hash("^="):
            case hash("~="):
            case hash("<<"):
            case hash(">>"):
                if ((two == "<<" || two == ">>") && pos + 2 < n && source[pos + 2] == '=') {
                    tokens.push_back(token(operator_, std::string(source.substr(pos, 3)), line, col));
                    pos += 3, col += 3;
                } else {
                    tokens.push_back(token(operator_, two, line, col));
                    pos += 2, col += 2;
                }
                continue;
            default:
                break;
            }
        }

        token_type typ;
        switch (c) {
        case '{': typ = lbrace; break;
        case '}': typ = rbrace; break;
        case '(': typ = lparen; break;
        case ')': typ = rparen; break;
        case '[': typ = lbracket; break;
        case ']': typ = rbracket; break;
        case ';': typ = semicolon; break;
        case ',': typ = comma; break;
        case ':': typ = colon; break;
        case '?': typ = question; break;
        case '.': typ = dot; break;
        default: typ = operator_; break;
        }

        tokens.push_back(token(typ, std::string(1, c), line, col));
        pos++, col++;
    }

    tokens.push_back(token(eof, "", line, col));
    return tokens;
}

} // namespace dwhbll::lang::c
