#pragma once

#include <span>
#include <string>
#include <vector>

namespace dwhbll::unicode {

    [[nodiscard]] auto to_utf8(std::span<const char32_t> str) -> std::string;
    [[nodiscard]] auto decode_wide(std::wstring_view str) -> std::u32string;
    [[nodiscard]] auto decode_utf16(std::u16string_view str) -> std::u32string;

    namespace normalization {
        [[nodiscard]] bool is_hangul_syllable(char32_t c);
        std::pair<std::array<char32_t, 3>, size_t> decompose_hangul(char32_t c);
        char32_t compose_hangul(char32_t first, char32_t second);

        /// @returns Empty vector if no decomposition matches
        std::vector<char32_t> decompose(char32_t c, bool accept_compat);

        void canonical_ordering(std::span<char32_t> spn);

        void canonical_composition(std::span<char32_t> &str);

        namespace nfc {
            std::u32string normalize(std::u32string_view str);

            bool quick_check(std::u32string_view str);
        }

        namespace nfd {
            std::u32string normalize(std::u32string_view str);

            bool quick_check(std::u32string_view str);
        }

        namespace nfkc {
            std::u32string normalize(std::u32string_view str);

            bool quick_check(std::u32string_view str);
        }

        namespace nfkd {
            std::u32string normalize(std::u32string_view str);

            bool quick_check(std::u32string_view str);
        }
    }
}
