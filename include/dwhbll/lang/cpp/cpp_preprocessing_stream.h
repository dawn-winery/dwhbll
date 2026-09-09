#pragma once

#include <generator>
#include <list>

#include <dwhbll/lang/common.h>
#include <dwhbll/lang/common/stream.h>
#include <dwhbll/lang/cpp/preprocess/line_splice_stream.h>

namespace dwhbll::lang::cpp {
    /*
[ [: // Also [::: -> [: :: and [:> -> [ :> disambig
< <% <: <= <=> <<= << // Also <::: -> <: :: and <::> -> <: :> disambig
% %= %> %: %:%:
:> : :: :]
... . .*
- -= -> ->* --
^^ ^ ^=
! !=
+ += ++
* *=
/ /=
> >= >>= >>
         */
    struct cpp_pp_token {
        enum OP_PUNC_TYPE {
            PUNC_NONE, ///< Sentinel for punc tree
            HASH, ///< #
            DOUBLE_HASH, ///< ##
            ALT_HASH, ///< %: (alternative for #)
            ALT_DOUBLE_HASH, ///< %:%: (alternative for ##)
            OPEN_CURLY, ///< {
            CLOSE_CURLY, ///< }
            ALT_OPEN_CURLY, ///< <% (alternative for {)
            ALT_CLOSE_CURLY, ///< %> (alternative for })
            OPEN_SQUARE, ///< [
            CLOSE_SQUARE, ///< ]
            ALT_OPEN_SQUARE, ///< <: (alternative for [)
            ALT_CLOSE_SQUARE, ///< :> (alternative for ])
            OPEN_PAREN, ///< (
            CLOSE_PAREN, ///< )
            OPEN_SPLICE, ///< [:
            CLOSE_SPLICE, ///< :]
            SEMICOLON, ///< ;
            COLON, ///< :
            ELLIPSIS, ///< ...
            QUESTION, ///< ?
            DOUBLE_COLON, ///< ::
            PERIOD, ///< .
            PERIOD_STAR, ///< .*
            ARROW, ///< ->
            ARROW_STAR, ///< ->*
            CAT_EARS, ///< ^^
            TILDE, ///< ~
            ALT_TILDE, ///< compl
            BANG, ///< !
            ALT_BANG, ///< not
            ADD, ///< +
            SUB, ///< -
            STAR, ///< *
            SLASH, ///< /
            PERCENT, ///< %
            HAT, ///< ^
            ALT_HAT, ///< xor
            AMP, ///< &
            ALT_AMP, ///< bitand
            PIPE, ///< |
            ALT_PIPE, ///< bitor
            EQ, ///< =
            ADD_EQ, ///< +=
            SUB_EQ, ///< -=
            STAR_EQ, ///< *=
            SLASH_EQ, ///< /=
            PERCENT_EQ, ///< %=
            HAT_EQ, ///< ^=
            ALT_HAT_EQ, ///< xor_eq
            AMP_EQ, ///< &=
            ALT_AMP_EQ, ///< and_eq
            PIPE_EQ, ///< |=
            ALT_PIPE_EQ, ///< or_eq
            EQ_EQ, ///< ==
            BANG_EQ, ///< !=
            ALT_BANG_EQ, ///< !=
            OPEN_ANGLE, ///< <
            CLOSE_ANGLE, ///< >
            OPEN_ANGLE_EQ, ///< <=
            CLOSE_ANGLE_EQ, ///> >=
            SPACESHIP, ///< <=>
            AMP_AMP, ///< &&
            ALT_AMP_AMP, ///< and
            PIPE_PIPE, ///< ||
            ALT_PIPE_PIPE, ///< or
            SHIFT_LEFT, ///< <<
            SHIFT_RIGHT, ///< >>
            SHIFT_LEFT_EQ, ///< <<=
            SHIFT_RIGHT_EQ, ///< >>=
            INCREMENT, ///< ++
            DECREMENT, ///< --
            COMMA, ///< ,
        };

        enum TYPE {
            NONE,
            WHITESPACE,
            IDENT,
            HEADER_NAME,
            PP_NUMBER,
            OP_PUNC,
            CHAR_UDL,
            STRING_UDL,
            RAW_STRING_UDL,
        } type;

        struct UDLString {
            std::u32string encoding_prefix;
            std::u32string body;
            std::u32string udl;
            std::u32string dchar{};
        };

        span spn;

        std::variant<std::u32string, OP_PUNC_TYPE, UDLString> data;

        [[nodiscard]] std::string to_string();

        [[nodiscard]] static std::string to_string(OP_PUNC_TYPE type);

        [[nodiscard]] static std::string to_string(const UDLString& type, TYPE ttype);
    };

    class cpp_preprocessing_stream {
        preprocess::line_splice_helper helper;

        files::filejar::fileid _file;

        struct merged_ws {
            bool is_ws;
            span spn;
            std::u32string buffer;
        };

        std::list<merged_ws> merged_whitespace;

        std::generator<cpp_pp_token> stream_pptokenize();

    public:
        cpp_preprocessing_stream(const files::filejar::fileid &file, std::unique_ptr<common::stream<char32_t, 16>> && stream);

        std::generator<cpp_pp_token> generator;
    };
}
