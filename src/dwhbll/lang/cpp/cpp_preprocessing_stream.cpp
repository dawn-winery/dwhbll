#include <stack>
#include <dwhbll/lang/cpp/cpp_preprocessing_stream.h>

#include <unordered_set>
#include <dwhbll/console/logging.h>

#include <dwhbll/lang/cpp/common.h>
#include <dwhbll/unicode/helpers.h>
#include <dwhbll/unicode/table.h>
#include <dwhbll/stl_ext/string.h>

namespace dwhbll::lang::cpp {
    enum cycle_state {
        CYCLE_NONE, ///< Nothing special
        CYCLE_CONSUME_ESCAPE, ///< Consume an escape sequence, decays into CYCLE_CONSUME_UCN
        CYCLE_CONSUME_ESCAPE_MAYBE_OPEN_BRACKET,
        CYCLE_CONSUME_ESCAPE_DIGITS,
        CYCLE_CONSUME_UCN, ///< Consume universal character name
        CYCLE_CONSUME_UCN_NAMED_BRACKET,
        CYCLE_CONSUME_UCN_DIGIT_BRACKET,
        CYCLE_CONSUME_UCN_HEX,
        CYCLE_CONSUME_UCN_CHARS,
        CYCLE_CONSUME_UCN_DIGIT_CLOSE,
        CYCLE_CONSUME_WHITESPACE, ///< Consuming whitespace
        CYCLE_CONSUME_LINE_COMMENT,
        CYCLE_CONSUME_C_COMMENT,
        CYCLE_MAYBE_C_COMMENT_END,
        CYCLE_CONSUME_IDENT,
        CYCLE_CONSUME_HEADER_NAME,
        CYCLE_CONSUME_RAW_STRING_DCHAR,
        CYCLE_CONSUME_RAW_STRING_BODY,
        CYCLE_CONSUME_RAW_STRING_DCHAR_END,
        CYCLE_CONSUME_STRING,
        CYCLE_CONSUME_CHAR,
        CYCLE_CONSUME_UDL_STRING, ///< User define literal after string
        CYCLE_CONSUME_UDL_CHAR, ///< User define literal after char
        CYCLE_CONSUME_UDL_RAW_STRING, ///< User define literal after raw string
        CYCLE_CONSUME_PPNUMBER, ///< Preprocessing number
        CYCLE_CONSUME_PPNUMBER_QUOTE,
        CYCLE_CONSUME_PPNUMBER_SIGN,
        CYCLE_DISAMBIG_HASH, ///< Token state #
        CYCLE_DISAMBIG_SQUARE_OPEN, ///< Token state [
        CYCLE_DISAMBIG_SQUARE_OPEN_COLON, ///< Token state [:
        CYCLE_DISAMBIG_SQUARE_OPEN_COLON_COLON, ///< Token state [:: (disambig for [:::)
        CYCLE_DISAMBIG_ANGLE_OPEN, ///< Token state <
        CYCLE_DISAMBIG_ANGLE_OPEN_EQ, ///< Token state <=
        CYCLE_DISAMBIG_ANGLE_OPEN_ANGLE_OPEN, ///< Token state <<
        CYCLE_DISAMBIG_ANGLE_OPEN_COLON, ///< Token state <:
        CYCLE_DISAMBIG_ANGLE_OPEN_COLON_COLON, ///< Token state <:: (disambig for <:::)
        CYCLE_DISAMBIG_PERCENT, ///< Token state %
        CYCLE_DISAMBIG_PERCENT_COLON, ///< Token state %:
        CYCLE_DISAMBIG_PERCENT_COLON_PERCENT, ///< Token state %:%
        CYCLE_DISAMBIG_COLON, ///< Token state :
        CYCLE_DISAMBIG_PERIOD, ///< Token state .
        CYCLE_DISAMBIG_PERIOD_PERIOD, ///< Token state ..
        CYCLE_DISAMBIG_MINUS, ///< Token state -
        CYCLE_DISAMBIG_MINUS_ANGLE_CLOSE, ///< ->
        CYCLE_DISAMBIG_HAT, ///< Token state ^
        CYCLE_DISAMBIG_BANG, ///< Token state !
        CYCLE_DISAMBIG_PLUS, ///< Token state +
        CYCLE_DISAMBIG_STAR, ///< Token state *
        CYCLE_DISAMBIG_SLASH, ///< Token state /
        CYCLE_DISAMBIG_AMP, ///< Token state &
        CYCLE_DISAMBIG_PIPE, ///< Token state |
        CYCLE_DISAMBIG_EQ, ///< Token state =
        CYCLE_DISAMBIG_ANGLE_CLOSE, ///< Token state >
        CYCLE_DISAMBIG_ANGLE_CLOSE_ANGLE_CLOSE, ///< Token state >>
    };

    const static std::unordered_map<std::u32string, cpp_pp_token::OP_PUNC_TYPE> reserved_ws {
        {U"and", cpp_pp_token::ALT_AMP_AMP},
        {U"bitand", cpp_pp_token::ALT_AMP},
        {U"and_eq", cpp_pp_token::ALT_AMP_EQ},
        {U"or", cpp_pp_token::ALT_PIPE_PIPE},
        {U"bitor", cpp_pp_token::ALT_PIPE},
        {U"or_eq", cpp_pp_token::ALT_PIPE_EQ},
        {U"xor", cpp_pp_token::ALT_HAT},
        {U"xor_eq", cpp_pp_token::ALT_HAT_EQ},
        {U"compl", cpp_pp_token::ALT_TILDE},
        {U"not", cpp_pp_token::ALT_BANG},
        {U"not_eq", cpp_pp_token::ALT_BANG_EQ},
    };

    const static std::unordered_set<std::u32string> strprefix {
        U"u8",
        U"u",
        U"U",
        U"L",
    };

    const static std::unordered_set<std::u32string> rstrprefix {
        U"R",
        U"u8R",
        U"uR",
        U"UR",
        U"LR",
    };

    std::string cpp_pp_token::to_string() {
        std::string result = "{(";

        result += std::format("{}({}:{}->{}:{})",
            spn.file.id,
            spn.begin().line,
            spn.begin().column,
            spn.end().line,
            spn.end().column
        );

        result += "), TYPE: ";

        switch (type) {
        case NONE:
            result += "UNKNOWN}: BODY: \"";
            result += stl_ext::utf8_encode(get<std::u32string>(data));
            result += '"';
            break;
        case WHITESPACE:
            result += "WHITESPACE}: BODY: \"";
            result += stl_ext::utf8_encode(get<std::u32string>(data));
            result += '"';
            break;
        case IDENT:
            result += "IDENT}: BODY: \"";
            result += stl_ext::utf8_encode(get<std::u32string>(data));
            result += '"';
            break;
        case HEADER_NAME:
            result += "HEADER_NAME}: BODY: \"";
            result += stl_ext::utf8_encode(get<std::u32string>(data));
            result += '"';
            break;
        case PP_NUMBER:
            result += "PP_NUMBER}: BODY: \"";
            result += stl_ext::utf8_encode(get<std::u32string>(data));
            result += '"';
            break;
        case OP_PUNC:
            result += "OP_PUNC}: BODY: \"";
            result += to_string(get<OP_PUNC_TYPE>(data));
            result += '"';
            break;
        case CHAR_UDL:
            result += "CHAR_UDL}: BODY: ";
            result += to_string(get<UDLString>(data), type);
            break;
        case STRING_UDL:
            result += "STRING_UDL}: BODY: ";
            result += to_string(get<UDLString>(data), type);
            break;
        case RAW_STRING_UDL:
            result += "RAW_STRING_UDL}: BODY: ";
            result += to_string(get<UDLString>(data), type);
            break;
        }

        return result;
    }
    std::string cpp_pp_token::to_string(OP_PUNC_TYPE type) {
        switch (type) {
        case PUNC_NONE:
            debug::todo();
        case HASH:
            return "#";
        case DOUBLE_HASH:
            return "##";
        case ALT_HASH:
            return "%:";
        case ALT_DOUBLE_HASH:
            return "%:%:";
        case OPEN_CURLY:
            return "{";
        case CLOSE_CURLY:
            return "}";
        case ALT_OPEN_CURLY:
            return "<%";
        case ALT_CLOSE_CURLY:
            return "%>";
        case OPEN_SQUARE:
            return "[";
        case CLOSE_SQUARE:
            return "]";
        case ALT_OPEN_SQUARE:
            return "<:";
        case ALT_CLOSE_SQUARE:
            return ":>";
        case OPEN_PAREN:
            return "(";
        case CLOSE_PAREN:
            return ")";
        case OPEN_SPLICE:
            return "[:";
        case CLOSE_SPLICE:
            return ":]";
        case SEMICOLON:
            return ";";
        case COLON:
            return ":";
        case ELLIPSIS:
            return "...";
        case QUESTION:
            return "?";
        case DOUBLE_COLON:
            return "::";
        case PERIOD:
            return ".";
        case PERIOD_STAR:
            return ".*";
        case ARROW:
            return "->";
        case ARROW_STAR:
            return "->*";
        case CAT_EARS:
            return "^^";
        case TILDE:
            return "~";
        case ALT_TILDE:
            return "compl";
        case BANG:
            return "!";
        case ALT_BANG:
            return "not";
        case ADD:
            return "+";
        case SUB:
            return "-";
        case STAR:
            return "*";
        case SLASH:
            return "/";
        case PERCENT:
            return "%";
        case HAT:
            return "^";
        case ALT_HAT:
            return "xor";
        case AMP:
            return "&";
        case ALT_AMP:
            return "bitand";
        case PIPE:
            return "|";
        case ALT_PIPE:
            return "bitor";
        case EQ:
            return "=";
        case ADD_EQ:
            return "+=";
        case SUB_EQ:
            return "-=";
        case STAR_EQ:
            return "*=";
        case SLASH_EQ:
            return "/=";
        case PERCENT_EQ:
            return "%=";
        case HAT_EQ:
            return "^=";
        case ALT_HAT_EQ:
            return "xor_eq";
        case AMP_EQ:
            return "&=";
        case ALT_AMP_EQ:
            return "and_eq";
        case PIPE_EQ:
            return "|=";
        case ALT_PIPE_EQ:
            return "or_eq";
        case EQ_EQ:
            return "==";
        case BANG_EQ:
            return "!=";
        case ALT_BANG_EQ:
            return "not_eq";
        case OPEN_ANGLE:
            return "<";
        case CLOSE_ANGLE:
            return ">";
        case OPEN_ANGLE_EQ:
            return "<=";
        case CLOSE_ANGLE_EQ:
            return ">=";
        case SPACESHIP:
            return "<=>";
        case AMP_AMP:
            return "&&";
        case ALT_AMP_AMP:
            return "and";
        case PIPE_PIPE:
            return "||";
        case ALT_PIPE_PIPE:
            return "or";
        case SHIFT_LEFT:
            return "<<";
        case SHIFT_RIGHT:
            return ">>";
        case SHIFT_LEFT_EQ:
            return "<<=";
        case SHIFT_RIGHT_EQ:
            return ">>=";
        case INCREMENT:
            return "++";
        case DECREMENT:
            return "--";
        case COMMA:
            return ",";
        }
        debug::unreachable();
    }

    std::string cpp_pp_token::to_string(const UDLString& type, TYPE ttype) {
        std::string result;

        result += stl_ext::utf8_encode(type.encoding_prefix);

        if (ttype == CHAR_UDL)
            result += '\'';
        else
            result += '"';

        if (ttype == RAW_STRING_UDL) {
            result += stl_ext::utf8_encode(type.dchar);
            result += '(';
        }

        result += stl_ext::utf8_encode(type.body);

        if (ttype == RAW_STRING_UDL) {
            result += ')';
            result += stl_ext::utf8_encode(type.dchar);
        }

        if (ttype == CHAR_UDL)
            result += '\'';
        else
            result += '"';

        result += stl_ext::utf8_encode(type.udl);

        return result;
    }

    std::generator<cpp_pp_token> cpp_preprocessing_stream::stream_pptokenize() {
        cycle_state cs{CYCLE_NONE};
        std::u32string strbuf1, strbuf2;
        std::uint64_t scratch1, scratch2;
        std::int64_t scratch3;
        span spn{_file}, spn_top{_file};
        bool consumed = false;
        std::stack<cycle_state> cs_stack;
        std::stack<span> spans;
        std::stack<std::u32string> strbuf_stack;
        std::list<cpp_pp_token> prepared;

        int consume_expect = 0;
        int counter = 0;

        while (helper.has_next()) {
            auto next = helper.next();

            if (cs == CYCLE_CONSUME_RAW_STRING_DCHAR) {
                spn_top.new_end(next.spn.end());
                if (next.is_line_splice)
                    debug::panic("Line splices may not appear in the delimiter character region of a raw string.");
                auto c = get<char32_t>(next.data);

                if (c == '(') {
                    cs = CYCLE_CONSUME_RAW_STRING_BODY;
                    continue;
                }

                if (c == ' ' || c == ')' || c == '\\' || c == '\t' || c == '\v' || c == '\f' || c == '\n')
                    debug::panic("Disallowed character encountered in raw string delimiter character region.");

                strbuf2 += c;
                continue;
            }
            if (cs == CYCLE_CONSUME_RAW_STRING_BODY) {
                spn_top.new_end(next.spn.end());
                if (next.is_line_splice) {
                    // line splice cannot possibly contain `)'
                    strbuf1 += get<std::u32string>(next.data);
                    continue;
                }

                auto c = get<char32_t>(next.data);

                if (c == ')') {
                    cs = CYCLE_CONSUME_RAW_STRING_DCHAR_END;
                    counter = 0;
                } else
                    strbuf1 += c;
                continue;
            }
            if (cs == CYCLE_CONSUME_RAW_STRING_DCHAR_END) {
                if (next.is_line_splice) {
                    // line splice cannot possibly be part of dchar end
                    spn_top.new_end(next.spn.end());
                    // fill missed section first before returning to collecting string body
                    strbuf1 += ')';
                    strbuf1 += strbuf2.substr(0, counter);
                    strbuf1 += get<std::u32string>(next.data);
                    continue;
                }

                auto c = get<char32_t>(next.data);

                if (counter == (int)strbuf2.size()) {
                    // expect quotation due to last
                    if (c == '"') {
                        // done consuming raw string body
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_UDL_RAW_STRING;
                    } else if (c == ')') {
                        // flush out our current attempt and try again
                        strbuf1 += ')';
                        strbuf1 += strbuf2.substr(0, counter);
                        counter = 0;
                    } else {
                        // failed to finish consuming, resume collecting body
                        strbuf1 += ')';
                        strbuf1 += strbuf2.substr(0, counter);
                        strbuf1 += c;
                        cs = CYCLE_CONSUME_RAW_STRING_BODY;
                    }
                } else {
                    // expect matching char as dchar-string
                    if (c == strbuf2[counter]) {
                        // increment counter
                        counter++;
                    } else if (c == ')') {
                        // flush out current attempt and try again
                        strbuf1 += ')';
                        strbuf1 += strbuf2.substr(0, counter);
                        counter = 0;
                    } else {
                        // failed to finish consuming, resume collecting body
                        strbuf1 += ')';
                        strbuf1 += strbuf2.substr(0, counter);
                        strbuf1 += c;
                        cs = CYCLE_CONSUME_RAW_STRING_BODY;
                    }
                }

                continue;
            }

            if (next.is_line_splice) {
                console::warn("{}", stl_ext::utf8_encode(get<std::u32string>(next.data)));
                continue;
            }

            spn = next.spn;
            auto c = get<char32_t>(next.data);

            consumed = false;

            while (!consumed) {
                switch (cs) {
                case CYCLE_NONE:
                    // begin production of a new token.
                    spn_top = spn;
                    strbuf1.clear();
                    if (cpp_is_ident_start(c)) {
                        strbuf1 += c;
                        cs = CYCLE_CONSUME_IDENT;
                        consumed = true;
                    } else if (cpp_is_whitespace(c)) {
                        strbuf1 += c;
                        cs = CYCLE_CONSUME_WHITESPACE;
                        consumed = true;
                    } else if (cpp_is_digit(c)) {
                        strbuf1 += c;
                        cs = CYCLE_CONSUME_PPNUMBER;
                        consumed = true;
                    } else switch (c) {
                    case '/':
                        // prepare '/' token
                        cs_stack.push(cs);
                        spans.push(spn);
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_SLASH;
                        consumed = true;
                        break;
                    case '#':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_HASH;
                        consumed = true;
                        break;
                    case '<':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_ANGLE_OPEN;
                        consumed = true;
                        break;
                    case '>':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_ANGLE_CLOSE;
                        consumed = true;
                        break;
                    case '=':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_EQ;
                        consumed = true;
                        break;
                    case '!':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_BANG;
                        consumed = true;
                        break;
                    case '|':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_PIPE;
                        consumed = true;
                        break;
                    case '&':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_AMP;
                        consumed = true;
                        break;
                    case '^':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_HAT;
                        consumed = true;
                        break;
                    case '\'':
                        // don't store the quote.
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_CHAR;
                        consumed = true;
                        break;
                    case '"':
                        // don't store the quote.
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_STRING;
                        consumed = true;
                        break;
                    case '.':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_PERIOD;
                        consumed = true;
                        break;
                    case '%':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_PERCENT;
                        consumed = true;
                        break;
                    case '*':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_STAR;
                        consumed = true;
                        break;
                    case ':':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_COLON;
                        consumed = true;
                        break;
                    case '+':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_PLUS;
                        consumed = true;
                        break;
                    case '-':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_MINUS;
                        consumed = true;
                        break;
                    case '\\':
                        // prepare to consume UCN
                        cs_stack.push(CYCLE_CONSUME_IDENT);
                        spans.push(spn);
                        prepared.emplace_back(cpp_pp_token::IDENT, spn, U"");
                        consumed = true;
                        cs = CYCLE_CONSUME_UCN;
                        break;
                    case '(':
                        // consume paren open directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::OPEN_PAREN};
                        consumed = true;
                        break;
                    case ')':
                        // consume paren close directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::CLOSE_PAREN};
                        consumed = true;
                        break;
                    case '{':
                        // consume curly open directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::OPEN_CURLY};
                        consumed = true;
                        break;
                    case '}':
                        // consume curly close directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::CLOSE_CURLY};
                        consumed = true;
                        break;
                    case '[':
                        strbuf1 += c;
                        cs = CYCLE_DISAMBIG_SQUARE_OPEN;
                        consumed = true;
                        break;
                    case ']':
                        // consume square close directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::CLOSE_SQUARE};
                        consumed = true;
                        break;
                    case ';':
                        // consume semicolon directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::SEMICOLON};
                        consumed = true;
                        break;
                    case '?':
                        // consume question mark directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::QUESTION};
                        consumed = true;
                        break;
                    case ',':
                        // consume comma directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::COMMA};
                        consumed = true;
                        break;
                    case '~':
                        // consume tilde directly
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::TILDE};
                        consumed = true;
                        break;
                    default:
                        if (!cpp_is_basic_char(c) || c == '\'' || c == '\"')
                            debug::panic("Malformed program, unexpected character {}", (char)c);
                        co_yield cpp_pp_token{cpp_pp_token::NONE, spn, std::u32string({c})};
                        consumed = true;
                    }
                    break;
                case CYCLE_CONSUME_UCN:
                    switch (c) {
                    case 'u':
                        // extend the span at the top of the stack.
                        spans.top().new_end(spn.end());
                        // not much more to do, just move on to DIGIT_BRACKET
                        cs = CYCLE_CONSUME_UCN_DIGIT_BRACKET;
                        consumed = true;
                        break;
                    case 'U':
                        scratch1 = 0;
                        spans.top().new_end(spn.end());
                        cs = CYCLE_CONSUME_UCN_HEX;
                        spn_top = spans.top();
                        spans.pop();
                        consumed = true;
                        consume_expect = 8;
                        break;
                    case 'N':
                        spans.top().new_end(spn.end());
                        cs = CYCLE_CONSUME_UCN_NAMED_BRACKET;
                        consumed = true;
                        break;
                    default:
                        // only can emit unknown token if the CS stack is for IDENT
                        if (cs_stack.top() == CYCLE_CONSUME_IDENT) {
                            co_yield cpp_pp_token{cpp_pp_token::NONE, spans.top(), std::u32string(U"\\")};
                            cs = CYCLE_NONE; // ignore what the source said about going back.
                            cs_stack.pop();
                            spans.pop();
                            prepared.pop_back();
                        } else
                            debug::panic(R"(Malformed escape in identifier, expected `\u', `\U', or `\N', found `\{}')", (char)c);
                        break;
                    }
                    break;
                case CYCLE_CONSUME_UCN_NAMED_BRACKET:
                    if (c != '{')
                        debug::panic("Malformed Universal Named Character, expected `\\N{{', found `\\N{}'", (char)c);
                    spans.top().new_end(spn.end());
                    cs = CYCLE_CONSUME_UCN_CHARS;
                    strbuf1.clear();
                    spn_top = spans.top();
                    spans.pop();
                    consumed = true;
                    break;
                case CYCLE_CONSUME_UCN_DIGIT_BRACKET:
                    strbuf1.clear();
                    scratch1 = 0;
                    if (c == '{') {
                        spans.top().new_end(spn.end());
                        cs = CYCLE_CONSUME_UCN_HEX;
                        spn_top = spans.top();
                        spans.pop();
                        consumed = true;
                        consume_expect = -1;
                    } else {
                        // we should expect 4 hex digits
                        spn_top = spans.top();
                        spans.pop();
                        consume_expect = 4;
                        cs = CYCLE_CONSUME_UCN_HEX;
                    }
                    break;
                case CYCLE_CONSUME_UCN_CHARS:
                    if (c == '\n')
                        debug::panic("Malformed Universal Named Character, expected `}}' before newline.");
                    if (c == '}') {
                        spn_top.new_end(spn.end());

                        // done consuming UCN
                        if (!unicode::aliases::name_aliases_to_codepoint.contains(strbuf1))
                            debug::panic("Malformed Universal Named Character, unknown Unicode alias for `{}'", stl_ext::utf8_encode(strbuf1));

                        auto ucnc = unicode::aliases::name_aliases_to_codepoint.at(strbuf1);

                        // restore context
                        cs = cs_stack.top();

                        // restore span
                        spn = prepared.back().spn;
                        spn.new_end(spn_top.end());
                        spn_top = spn;

                        strbuf1 = get<std::u32string>(prepared.back().data);
                        strbuf1 += ucnc;
                        consumed = true;

                        // clear stack states
                        cs_stack.pop();
                        prepared.pop_back();
                    } else {
                        spn_top.new_end(spn.end());

                        // consume additional char
                        strbuf1 += c;
                        consumed = true;
                    }
                    break;
                case CYCLE_CONSUME_UCN_HEX:
                    if (consume_expect < 0 && c == '}') {
                        spn_top.new_end(spn.end());

                        // finished consuming UCN
                        if (consume_expect < -8)
                            debug::panic("Consumed too many hex digits, should be at most 8 digits, consumed {}!", -consume_expect - 1);

                        // check that the data is remotely valid
                        if (scratch1 > 0xD7FF && scratch1 < 0xE000)
                            debug::panic("Consumed universal character, U+{:x}, is a unicode surrogate!", scratch1);

                        if (scratch1 > 0x10FFFF)
                            debug::panic("Consumed universal character, U+{:x}, which is not in the unicode range!", scratch1);

                        // clear scratch 2 just in case
                        scratch2 = 0;

                        // restore context
                        cs = cs_stack.top();

                        // restore span
                        spn = prepared.back().spn;
                        spn.new_end(spn_top.end());
                        spn_top = spn;

                        strbuf1 = get<std::u32string>(prepared.back().data);
                        strbuf1 += (char32_t)scratch1;

                        // clear stack states
                        cs_stack.pop();
                        prepared.pop_back();
                    }

                    if (cpp_is_digit(c)) {
                        scratch2 = c - '0';
                    } else if (c >= 'a' && c <= 'f') {
                        scratch2 = c - 'a' + 10;
                    } else if (c >= 'A' && c <= 'F') {
                        scratch2 = c - 'A' + 10;
                    } else
                        debug::panic("Expected a hex digit in universal character, got {}!", (char)c);

                    scratch1 = scratch1 * 16 + scratch2;
                    consume_expect--;
                    consumed = true;

                    if (consume_expect == 0) {
                        // done consuming
                        // check that the data is remotely valid
                        if (scratch1 > 0xD7FF && scratch1 < 0xE000)
                            debug::panic("Consumed universal character, U+{:x}, is a unicode surrogate!", scratch1);

                        if (scratch1 > 0x10FFFF)
                            debug::panic("Consumed universal character, U+{:x}, which is not in the unicode range!", scratch1);

                        spn_top.new_end(spn.end());

                        // clear scratch 2 just in case
                        scratch2 = 0;

                        // restore context
                        cs = cs_stack.top();

                        // restore span
                        spn = prepared.back().spn;
                        spn.new_end(spn_top.end());
                        spn_top = spn;

                        strbuf1 = get<std::u32string>(prepared.back().data);
                        strbuf1 += (char32_t)scratch1;

                        // clear stack states
                        cs_stack.pop();
                        prepared.pop_back();
                    }
                    break;
                case CYCLE_CONSUME_UCN_DIGIT_CLOSE:
                    debug::panic("Unhandled character {}", (char)c);
                case CYCLE_CONSUME_WHITESPACE:
                    if (cpp_is_whitespace(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else if (c == '/') {
                        // prepare a whitespace token just in case
                        cs_stack.push(cs);
                        spans.push(spn_top);
                        strbuf1 += c;
                        prepared.emplace_back(cpp_pp_token::WHITESPACE, spn_top, strbuf1);
                        cs = CYCLE_DISAMBIG_SLASH;
                        consumed = true;
                    } else {
                        co_yield cpp_pp_token{cpp_pp_token::WHITESPACE, spn_top, strbuf1};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_CONSUME_LINE_COMMENT:
                    if (c == '\n') {
                        // end of comment
                        cs = CYCLE_CONSUME_WHITESPACE;
                    }

                    strbuf1 += c;
                    spn_top.new_end(spn.end());
                    consumed = true;
                    break;
                case CYCLE_CONSUME_C_COMMENT:
                    if (c == '*') {
                        // send off to check if end of comment
                        cs = CYCLE_MAYBE_C_COMMENT_END;
                    }

                    strbuf1 += c;
                    spn_top.new_end(spn.end());
                    consumed = true;
                    break;
                case CYCLE_MAYBE_C_COMMENT_END:
                    // check if matching end of comment
                    if (c == '/')
                        cs = CYCLE_CONSUME_WHITESPACE;
                    else
                        cs = CYCLE_CONSUME_C_COMMENT;

                    strbuf1 += c;
                    spn_top.new_end(spn.end());
                    consumed = true;
                    break;
                case CYCLE_CONSUME_IDENT:
                    if (cpp_is_ident_continue(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else if (c == '"' && strprefix.contains(strbuf1)) {
                        // string.
                        // don't store the quote
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_STRING;
                        consumed = true;
                    } else if (c == '"' && rstrprefix.contains(strbuf1)) {
                        // string.
                        // don't store the quote
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        strbuf2.clear(); // clear the dchar region buffer
                        cs = CYCLE_CONSUME_RAW_STRING_DCHAR;
                        consumed = true;
                    } else if (c == '\'' && strprefix.contains(strbuf1)) {
                        // don't store the quote
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_CHAR;
                        consumed = true;
                    } else if (c == '\\') {
                        // prepare to consume UCN
                        cs_stack.push(cs);
                        spans.push(spn_top);
                        prepared.emplace_back(cpp_pp_token::IDENT, spn_top, strbuf1);
                        consumed = true;
                        cs = CYCLE_CONSUME_UCN;
                        break;
                    } else {
                        // produce ident
                        if (!unicode::normalization::nfc::quick_check(strbuf1))
                            debug::panic("At {}({}:{}->{}:{}), identifier {} does not conform to Unicode Normalization Form C!",
                                spn_top.file.id,
                                spn_top.begin().line,
                                spn_top.begin().column,
                                spn_top.end().line,
                                spn_top.end().column,
                                stl_ext::utf8_encode(strbuf1)
                            );
                        if (auto it = reserved_ws.find(strbuf1); it != reserved_ws.end())
                            co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, it->second};
                        else
                            co_yield cpp_pp_token{cpp_pp_token::IDENT, spn_top, strbuf1};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_CONSUME_HEADER_NAME:
                    debug::panic("Unhandled character {}", (char)c);
                case CYCLE_CONSUME_ESCAPE:
                    if (c == 'u' || c == 'U' || c == 'N') {
                        // UCN
                        spans.push(spn_top);
                        prepared.emplace_back(cpp_pp_token::IDENT, spn_top, strbuf1);
                        cs = CYCLE_CONSUME_UCN;
                        break;
                    } else if (c == 'o' || c == 'x' || cpp_is_digit(c)) {
                        if (c == 'x')
                            scratch1 = 16; // hex numbers
                        else
                            scratch1 = 8; // octal numbers
                        scratch3 = -1; // unknown length
                        cs = CYCLE_CONSUME_ESCAPE_MAYBE_OPEN_BRACKET;
                        if (c >= '0' && c <= '7') {
                            // directly start consuming digits
                            // put the digit into scratch2
                            scratch2 = c - '0';
                            scratch3 = 2; // expect at most 2 more digits
                            cs = CYCLE_CONSUME_ESCAPE_DIGITS;
                        } else
                            scratch2 = 0;
                        consumed = true;
                        break;
                    } else switch (c) {
                    case '\'':
                        strbuf1 += '\'';
                        break;
                    case '"':
                        strbuf1 += '"';
                        break;
                    case '?':
                        strbuf1 += '?';
                        break;
                    case '\\':
                        strbuf1 += '\\';
                        break;
                    case 'a':
                        strbuf1 += '\a';
                        break;
                    case 'b':
                        strbuf1 += '\b';
                        break;
                    case 'f':
                        strbuf1 += '\f';
                        break;
                    case 'n':
                        strbuf1 += '\n';
                        break;
                    case 'r':
                        strbuf1 += '\r';
                        break;
                    case 't':
                        strbuf1 += '\t';
                        break;
                    case 'v':
                        strbuf1 += '\v';
                        break;
                    default:
                        debug::panic("Unhandled character {}", (char)c);
                    }
                    spn_top.new_end(spn.end());
                    consumed = true;
                    cs = cs_stack.top();
                    cs_stack.pop();
                    break;
                case CYCLE_CONSUME_ESCAPE_DIGITS:
                    if (scratch3 > 0) {
                        // octal digit with limits
                        if (c >= '0' && c <= '7') {
                            spn_top.new_end(spn.end());
                            consumed = true;
                            scratch2 = scratch2 * scratch1 + (c - '0');
                            scratch3--;
                        }

                        if (scratch3 == 0 || c < '0' || c > '7') {
                            scratch1 = 0;
                            scratch3 = 0;

                            strbuf1 += (char32_t)scratch2;

                            // restore cs
                            cs = cs_stack.top();
                            cs_stack.pop();
                        }
                    } else {
                        // consuming hex
                        // TODO: validate correctness of the char!
                        if (scratch1 == 16) {
                            if (c >= '0' && c <= '9') {
                                spn_top.new_end(spn.end());
                                scratch2 = scratch2 * scratch1 + (c - '0');
                                consumed = true;
                            } else if (c >= 'a' && c <= 'f') {
                                spn_top.new_end(spn.end());
                                scratch2 = scratch2 * scratch1 + (c - 'a' + 10);
                                consumed = true;
                            } else if (c >= 'A' && c <= 'F') {
                                spn_top.new_end(spn.end());
                                scratch2 = scratch2 * scratch1 + (c - 'A' + 10);
                                consumed = true;
                            } else if (scratch3 == 0 || (scratch3 == -1 && c == '}')) {
                                // end
                                if (scratch3 == -1 && c == '}')
                                    consumed = true;

                                scratch1 = 0;
                                scratch3 = 0;

                                if (scratch2 > 0x10FFF)
                                    debug::panic("Character U+{:x} cannot be encoded!", scratch2);
                                strbuf1 += (char32_t)scratch2;

                                // restore cs
                                cs = cs_stack.top();
                                cs_stack.pop();
                            } else {
                                debug::panic("Expected hex digits or closing bracket when consuming escape! (got `{}')", (char)c);
                            }
                        } else if (scratch1 == 8) {
                            if (c >= '0' && c <= '7') {
                                spn_top.new_end(spn.end());
                                scratch2 = scratch2 * scratch1 + (c - '0');
                                consumed = true;
                            } else if (c == '}') {
                                // no need to special case the scratch3 == -1,
                                // raw octal digits has max of 3 digits, case
                                // already considered
                                consumed = true;

                                scratch1 = 0;
                                scratch3 = 0;

                                strbuf1 += (char32_t)scratch2;

                                // restore cs
                                cs = cs_stack.top();
                                cs_stack.pop();
                            } else {
                                debug::panic("Expected octal digits or closing bracket when consuming escape! (got `{}')", (char)c);
                            }
                        } else
                            debug::unreachable();
                    }
                    break;
                case CYCLE_CONSUME_ESCAPE_MAYBE_OPEN_BRACKET:
                    if (c == '{') {
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else
                        scratch3 = 0;
                    cs = CYCLE_CONSUME_ESCAPE_DIGITS;
                    break;
                case CYCLE_CONSUME_RAW_STRING_DCHAR:
                case CYCLE_CONSUME_RAW_STRING_BODY:
                case CYCLE_CONSUME_RAW_STRING_DCHAR_END:
                    debug::panic("Reached Unreachable with char {}!", (char)c);
                case CYCLE_CONSUME_STRING:
                    if (c == '"') {
                        // done consuming string
                        spn_top.new_end(spn.end());
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_UDL_STRING;
                        consumed = true;
                    } else if (c == '\\') {
                        cs_stack.push(cs);
                        spn_top.new_end(spn.end());
                        cs = CYCLE_CONSUME_ESCAPE;
                        consumed = true;
                    } else {
                        // store string body into strbuf2
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    }
                    break;
                case CYCLE_CONSUME_CHAR:
                    if (c == '\'') {
                        // done consuming char
                        spn_top.new_end(spn.end());
                        strbuf_stack.push(strbuf1);
                        strbuf1.clear();
                        cs = CYCLE_CONSUME_UDL_CHAR;
                        consumed = true;
                    } else if (c == '\\') {
                        cs_stack.push(cs);
                        spn_top.new_end(spn.end());
                        cs = CYCLE_CONSUME_ESCAPE;
                        consumed = true;
                    } else {
                        // store char body into strbuf2
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    }
                    break;
                case CYCLE_CONSUME_UDL_STRING:
                    if (cpp_is_ident_continue(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else {
                        // produce string token
                        if (!unicode::normalization::nfc::quick_check(strbuf1))
                            debug::panic("At {}({}:{}->{}:{}), user defined literal suffix `{}' does not conform to Unicode Normalization Form C!",
                                spn_top.file.id,
                                spn_top.begin().line,
                                spn_top.begin().column,
                                spn_top.end().line,
                                spn_top.end().column,
                                stl_ext::utf8_encode(strbuf1)
                            );
                        auto body = strbuf_stack.top();
                        strbuf_stack.pop();
                        auto prefix = strbuf_stack.top();
                        strbuf_stack.pop();
                        co_yield cpp_pp_token{cpp_pp_token::STRING_UDL, spn_top, cpp_pp_token::UDLString {
                            prefix,
                            body,
                            strbuf1
                        }};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_CONSUME_UDL_CHAR:
                    if (cpp_is_ident_continue(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else {
                        // produce string token
                        if (!unicode::normalization::nfc::quick_check(strbuf1))
                            debug::panic("At {}({}:{}->{}:{}), user defined literal suffix `{}' does not conform to Unicode Normalization Form C!",
                                spn_top.file.id,
                                spn_top.begin().line,
                                spn_top.begin().column,
                                spn_top.end().line,
                                spn_top.end().column,
                                stl_ext::utf8_encode(strbuf1)
                            );
                        auto body = strbuf_stack.top();
                        strbuf_stack.pop();
                        auto prefix = strbuf_stack.top();
                        strbuf_stack.pop();
                        co_yield cpp_pp_token{cpp_pp_token::CHAR_UDL, spn_top, cpp_pp_token::UDLString {
                            prefix,
                            body,
                            strbuf1
                        }};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_CONSUME_UDL_RAW_STRING:
                    if (cpp_is_ident_continue(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else {
                        // produce string token
                        if (!unicode::normalization::nfc::quick_check(strbuf1))
                            debug::panic("At {}({}:{}->{}:{}), user defined literal suffix `{}' does not conform to Unicode Normalization Form C!",
                                spn_top.file.id,
                                spn_top.begin().line,
                                spn_top.begin().column,
                                spn_top.end().line,
                                spn_top.end().column,
                                stl_ext::utf8_encode(strbuf1)
                            );
                        auto body = strbuf_stack.top();
                        strbuf_stack.pop();
                        auto prefix = strbuf_stack.top();
                        strbuf_stack.pop();
                        co_yield cpp_pp_token{cpp_pp_token::RAW_STRING_UDL, spn_top, cpp_pp_token::UDLString {
                            prefix,
                            body,
                            strbuf1,
                            strbuf2
                        }};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_CONSUME_PPNUMBER:
                    // Ident continue is a superset of all digits
                    if (c == '\'') {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_CONSUME_PPNUMBER_QUOTE;
                    } else if (c == 'e' || c == 'E' || c == 'p' || c == 'P') {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_CONSUME_PPNUMBER_SIGN;
                    } else if (c == '.' || cpp_is_ident_continue(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    } else {
                        // guaranteed end of pp-number.
                        co_yield cpp_pp_token{cpp_pp_token::PP_NUMBER, spn_top, strbuf1};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_CONSUME_PPNUMBER_QUOTE:
                    if (cpp_is_digit(c) || cpp_is_nondigit(c)) {
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_CONSUME_PPNUMBER;
                    } else {
                        debug::panic("At {}({}:{}->{}:{}), after `\\'' in number, expected a digit or non-digit, got `{}'.",
                            spn_top.file.id,
                            spn_top.begin().line,
                            spn_top.begin().column,
                            spn_top.end().line,
                            spn_top.end().column,
                            (char)c
                        );
                    }
                    break;
                case CYCLE_CONSUME_PPNUMBER_SIGN:
                    if (c == '+' || c == '-') {
                        // it doesn't need to be a sign.
                        strbuf1 += c;
                        spn_top.new_end(spn.end());
                        consumed = true;
                    }
                    cs = CYCLE_CONSUME_PPNUMBER;
                    break;
                case CYCLE_DISAMBIG_HASH:
                    if (c == '#') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DOUBLE_HASH};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::HASH};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_SQUARE_OPEN:
                    // [ [: // Also [::: -> [: :: and [:> -> [ :> disambig
                    if (c == ':') {
                        // due to disambiguation, push the span for the `[`
                        // start fresh span for the `:`
                        spans.push(spn_top);
                        spn_top = spn;
                        consumed = true;
                        cs = CYCLE_DISAMBIG_SQUARE_OPEN_COLON;
                    } else {
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::OPEN_SQUARE};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_DISAMBIG_SQUARE_OPEN_COLON:
                    // [:
                    // [::: -> [: :: and [:> -> [ :> disambig
                    if (c == ':') {
                        // due to disambiguation, push the span for the first `:`
                        // start fresh span for the second `:`
                        spans.push(spn_top);
                        spn_top = spn;
                        consumed = true;
                        cs = CYCLE_DISAMBIG_SQUARE_OPEN_COLON_COLON;
                    } else if (c == '>') {
                        // disambiguation for [:>
                        // span for the `[` is on the stack
                        spn_top.new_end(spn.end());

                        spn = spans.top();
                        spans.pop();

                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::OPEN_SQUARE};
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_CLOSE_SQUARE};
                        consumed = true;
                        cs = CYCLE_NONE;
                    } else {
                        // just [:
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::OPEN_SPLICE};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_DISAMBIG_SQUARE_OPEN_COLON_COLON:
                    if (c == ':') {
                        // standard case, produce the token [: then ::
                        // the last two `:` are in spn_top and spn
                        spn_top.new_end(spn.end());

                        // shuffle some stuff around to bring the span for `[:`
                        // into spn
                        spn = spans.top();
                        spans.pop();
                        spans.top().new_end(spn.end());
                        spn = spans.top();
                        spans.pop();

                        // produce the two tokens
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::OPEN_SPLICE};
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DOUBLE_COLON};
                        consumed = true;
                    } else {
                        // special case, produce token `[` followed by token `::`
                        // we cannot clobber spn because we don't use it.
                        // join the spans for the `::` token.
                        spans.top().new_end(spn_top.end());
                        spn_top = spans.top();
                        spans.pop();

                        // produce the two tokens
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spans.top(), cpp_pp_token::OPEN_SQUARE};
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DOUBLE_COLON};
                        spans.pop();
                        // did not consume
                    }
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_ANGLE_OPEN:
                    // < <% <: <= <=> <<= <<    Also <::: -> <: :: and <::> -> <: :> disambig
                    switch (c) {
                    case ':':
                        // due to disambiguation, push the span for the `<`
                        // start fresh span for :
                        spans.push(spn_top);
                        spn_top = spn;
                        consumed = true;
                        cs = CYCLE_DISAMBIG_ANGLE_OPEN_COLON;
                        break;
                    case '%':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_OPEN_CURLY};
                        cs = CYCLE_NONE;
                        break;
                    case '=':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_DISAMBIG_ANGLE_OPEN_EQ;
                        break;
                    case '<':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_DISAMBIG_ANGLE_OPEN_ANGLE_OPEN;
                        break;
                    default:
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::OPEN_ANGLE};
                        cs = CYCLE_NONE;
                        break;
                    }
                    break;
                case CYCLE_DISAMBIG_ANGLE_OPEN_EQ:
                    if (c == '>') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SPACESHIP};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::OPEN_ANGLE_EQ};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_ANGLE_OPEN_ANGLE_OPEN:
                    // <<= <<
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SHIFT_LEFT_EQ};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SHIFT_LEFT};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_ANGLE_OPEN_COLON:
                    // <:, <::: -> <: :: and <::> into <: :>
                    if (c == ':') {
                        // continue disambiguation
                        // due to disambiguation, push the span for the `:`
                        // start fresh span for the next :
                        spans.push(spn_top);
                        spn_top = spn;
                        consumed = true;
                        cs = CYCLE_DISAMBIG_ANGLE_OPEN_COLON_COLON;
                    } else {
                        // alternative [ token.
                        // the span for the `<` is in the stack, merge them
                        spn = spans.top();
                        spans.pop();
                        spn.new_end(spn_top.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::ALT_OPEN_SQUARE};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_DISAMBIG_ANGLE_OPEN_COLON_COLON:
                    if (c == ':') {
                        // produce <: then :: after seeing <:::
                        consumed = true;

                        // first, join the last two colons, the span for the
                        // first is in spn_top
                        spn_top.new_end(spn.end());

                        // then, use spn as scratch space to merge the bottom 2 spans
                        spn = spans.top();
                        spans.pop();
                        spans.top().new_end(spn.end());
                        spn = spans.top();
                        spans.pop();

                        // emit the two tokens
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::ALT_OPEN_SQUARE};
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DOUBLE_COLON};
                    } else if (c == '>') {
                        // produce <: then :> after seeing <::>
                        consumed = true;

                        // first, join the last colon and angle bracket, the
                        // span for the colon is in spn_top
                        spn_top.new_end(spn.end());

                        // then, use spn as scratch space to merge the bottom 2 spans
                        spn = spans.top();
                        spans.pop();
                        spans.top().new_end(spn.end());
                        spn = spans.top();
                        spans.pop();

                        // emit the two tokens
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::ALT_OPEN_SQUARE};
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_CLOSE_SQUARE};
                    } else {
                        // produce <, then ::, then switch to CYCLE_NONE without consuming anything
                        // first, join the last two colons
                        spans.top().new_end(spn_top.end());

                        spn_top = spans.top();
                        spans.pop();

                        // then, get the span for the open angle bracket
                        spn = spans.top();
                        spans.pop();

                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn, cpp_pp_token::OPEN_ANGLE};
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DOUBLE_COLON};
                    }
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_PERCENT:
                    // % %= %> %: %:%:
                    switch (c) {
                    case '=':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::PERCENT_EQ};
                        cs = CYCLE_NONE;
                        break;
                    case '>':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_CLOSE_CURLY};
                        cs = CYCLE_NONE;
                        break;
                    case ':':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_DISAMBIG_PERCENT_COLON;
                        break;
                    default:
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::PERCENT};
                        cs = CYCLE_NONE;
                        break;
                    }
                    break;
                case CYCLE_DISAMBIG_PERCENT_COLON:
                    // %: or %:%:
                    if (c == '%') {
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_DISAMBIG_PERCENT_COLON_PERCENT;
                    } else {
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_HASH};
                        cs = CYCLE_NONE;
                    }
                    break;
                case CYCLE_DISAMBIG_PERCENT_COLON_PERCENT:
                    if (c == ':') {
                        spn_top.new_end(spn.end());
                        consumed = true;
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_DOUBLE_HASH};
                        cs = CYCLE_NONE;
                    } else {
                        debug::panic("At {}({}:{}->{}:{}), after `%:%', expected `:' to match `%:%:', got `{}'.",
                            spn_top.file.id,
                            spn_top.begin().line,
                            spn_top.begin().column,
                            spn_top.end().line,
                            spn_top.end().column,
                            (char)c
                        );
                    }
                    break;
                case CYCLE_DISAMBIG_COLON:
                    //:> : :: :]
                    consumed = true;
                    switch (c) {
                    case '>':
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ALT_CLOSE_SQUARE};
                        break;
                    case ':':
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DOUBLE_COLON};
                        break;
                    case ']':
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::CLOSE_SPLICE};
                        break;
                    default:
                        consumed = false;
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::COLON};
                        break;
                    }
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_PERIOD:
                    //... . .*
                    if (cpp_is_digit(c)) {
                        // disambiguated as pp-number
                        spn_top.new_end(spn.end());
                        cs = CYCLE_CONSUME_PPNUMBER;
                        strbuf1 += c;
                        consumed = true;
                    } else if (c == '.') {
                        spn_top.new_end(spn.end());
                        cs = CYCLE_DISAMBIG_PERIOD_PERIOD;
                        strbuf1 += c;
                        consumed = true;
                    } else if (c == '*') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::PERIOD_STAR};
                        cs = CYCLE_NONE;
                    } else {
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::PERIOD};
                        cs = CYCLE_NONE;
                    }
                    break;
                    debug::panic("Unhandled character {}", (char)c);
                case CYCLE_DISAMBIG_PERIOD_PERIOD:
                    if (c == '.') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ELLIPSIS};
                        consumed = true;
                    } else
                        debug::panic("At {}({}:{}->{}:{}), expected `.' after `..'",
                            spn_top.file.id,
                            spn_top.begin().line,
                            spn_top.begin().column,
                            spn_top.end().line,
                            spn_top.end().column
                        );
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_MINUS:
                    // - -= -> ->* --
                    // leaving this regardless
                    cs = CYCLE_NONE;
                    switch (c) {
                    case '=':
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SUB_EQ};
                        consumed = true;
                        break;
                    case '-':
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::DECREMENT};
                        consumed = true;
                        break;
                    case '>':
                        spn_top.new_end(spn.end());
                        cs = CYCLE_DISAMBIG_MINUS_ANGLE_CLOSE;
                        consumed = true;
                        break;
                    default:
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SUB};
                        break;
                    }
                    break;
                case CYCLE_DISAMBIG_MINUS_ANGLE_CLOSE:
                    // -> ->*
                    if (c == '*') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ARROW_STAR};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ARROW};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_HAT:
                    if (c == '^') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::CAT_EARS};
                        consumed = true;
                    } else if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::HAT_EQ};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::HAT};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_BANG:
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::BANG_EQ};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::BANG};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_PLUS:
                    // + += ++
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ADD_EQ};
                        consumed = true;
                    } else if (c == '+') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::INCREMENT};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::ADD};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_STAR:
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::STAR_EQ};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::STAR};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_SLASH:
                    switch (c) {
                    case '*':
                        strbuf1 += c;
                        // use the spn size from spans, in case coming from whitespace
                        spn_top = spans.top();
                        // eat prepared token, we don't need it.
                        if (!prepared.empty())
                            prepared.pop_back();
                        spans.pop();
                        cs_stack.pop();
                        // change to consuming C style comment
                        cs = CYCLE_CONSUME_C_COMMENT;
                        consumed = true;
                        break;
                    case '/':
                        strbuf1 += c;
                        // use the spn size from spans, in case coming from whitespace
                        spn_top = spans.top();
                        // eat prepared token, we don't need it.
                        if (!prepared.empty())
                            prepared.pop_back();
                        spans.pop();
                        cs_stack.pop();
                        // change to consuming line comment
                        cs = CYCLE_CONSUME_LINE_COMMENT;
                        consumed = true;
                        break;
                    case '=':
                        // emit prepared token, if any
                        if (!prepared.empty()) {
                            co_yield prepared.back();
                            prepared.pop_back();
                        }
                        spans.pop();
                        cs_stack.pop();
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SLASH_EQ};
                        consumed = true;
                        cs = CYCLE_NONE;
                        break;
                    default:
                        // emit prepared token, if any
                        if (!prepared.empty()) {
                            co_yield prepared.back();
                            prepared.pop_back();
                        }
                        spans.pop();
                        cs_stack.pop();
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SLASH};
                        cs = CYCLE_NONE;
                        break;
                    }
                    break;
                case CYCLE_DISAMBIG_AMP:
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::AMP_EQ};
                        consumed = true;
                    } else if (c == '|') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::AMP_AMP};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::EQ};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_PIPE:
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::PIPE_EQ};
                        consumed = true;
                    } else if (c == '|') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::PIPE_PIPE};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::EQ};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_EQ:
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::EQ_EQ};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::EQ};
                    cs = CYCLE_NONE;
                    break;
                case CYCLE_DISAMBIG_ANGLE_CLOSE:
                    // > >= >>= >>
                    switch (c) {
                    case '>':
                        spn_top.new_end(spn.end());
                        consumed = true;
                        cs = CYCLE_DISAMBIG_ANGLE_CLOSE_ANGLE_CLOSE;
                        break;
                    case '=':
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::CLOSE_ANGLE_EQ};
                        consumed = true;
                        cs = CYCLE_NONE;
                        break;
                    default:
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::CLOSE_ANGLE};
                        cs = CYCLE_NONE;
                        break;
                    }
                    break;
                case CYCLE_DISAMBIG_ANGLE_CLOSE_ANGLE_CLOSE:
                    if (c == '=') {
                        spn_top.new_end(spn.end());
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SHIFT_RIGHT_EQ};
                        consumed = true;
                    } else
                        co_yield cpp_pp_token{cpp_pp_token::OP_PUNC, spn_top, cpp_pp_token::SHIFT_RIGHT};
                    cs = CYCLE_NONE;
                    break;
                }
            }
        }

        if (!prepared.empty()) {
            console::info("vvvvvvvvvv [ADDITIONAL PANIC INFORMATION] vvvvvvvvvv");
            while (!prepared.empty()) {
                auto v = prepared.back();
                prepared.pop_back();
                console::info("{}", v.to_string());
            }
            debug::panic("Failed to fully emit all prepared tokens by the time state machine exited!");
        }

        if (!cs_stack.empty() || !spans.empty())
            debug::panic("Mismatched consumption of CS states and spans!");

        if (cs != CYCLE_NONE && cs != CYCLE_CONSUME_WHITESPACE) // whitespace is acceptable
            debug::panic("Exited tokenizer with a tokenizer state that is not acceptable, got {}!", static_cast<int>(cs));

        if (cs == CYCLE_CONSUME_WHITESPACE) // produce final whitespace
            co_yield cpp_pp_token{cpp_pp_token::WHITESPACE, spn_top, strbuf1};
    }

    cpp_preprocessing_stream::cpp_preprocessing_stream(
        const files::filejar::fileid &file,
        std::unique_ptr<common::stream<char32_t, 16>> &&stream) :
        helper(file, std::move(stream)),
        _file(file),
        generator(stream_pptokenize()
    ) {}
}
