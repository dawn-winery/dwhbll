#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <sstream>

// TODO: rest of ANSI escapes maybe?
namespace dwhbll::console::ansi_escape {
    enum class Graphics {
        RESET = 0,
        BOLD = 1,
        FAINT = 2,
        ITALIC = 3,
        UNDERLINE = 4,
        BLINK_SLOW = 5,
        BLINK_FAST = 6,
        INVERT = 7,
        HIDE = 8, ///< Not widely supported.
        STRIKETHROUGH = 9,
        FONT_DEFAULT = 10,
        FONT_ALT_1 = 11,
        FONT_ALT_2 = 12,
        FONT_ALT_3 = 13,
        FONT_ALT_4 = 14,
        FONT_ALT_5 = 15,
        FONT_ALT_6 = 16,
        FONT_ALT_7 = 17,
        FONT_ALT_8 = 18,
        FONT_ALT_9 = 19,
        FONT_GOTHIC = 20, ///< Not widely supported.
        DOUBLE_UNDERLINE = 21,
        CLEAR_INTENSITY = 22, ///< Reset Bold/Faint status
        CLEAR_ITALIC = 23, ///< Reset italic and blackletter status
        CLEAR_UNDERLINED = 24, ///< Reset underlined (including double) status
        CLEAR_BLINKING = 25, ///< Reset blinking status
        CLEAR_REVERSED = 27, ///< Reset reversed state
        FG_BLACK = 30,
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW = 33,
        FG_BLUE = 34,
        FG_MAGENTA = 35,
        FG_CYAN = 36,
        FG_WHITE = 37,
        FG_OTHER = 38, ///< Set foreground color (8 or 24 bit color)
        FG_DEFAULT = 39,
        BG_BLACK = 40,
        BG_RED = 41,
        BG_GREEN = 42,
        BG_YELLOW = 43,
        BG_BLUE = 44,
        BG_MAGENTA = 45,
        BG_CYAN = 46,
        BG_WHITE = 47,
        BG_OTHER = 48, ///< Set background color (8 or 24 bit color)
        BG_DEFAULT = 49,
        CLEAR_PROPORTIONAL_SPACING = 50,
        FRAMED = 51,
        CIRCLED = 52,
        OVERLINE = 53,
        CLEAR_FRAMING = 54,
        CLEAR_OVERLINE = 55,
        UNDERLINE_OTHER = 58, ///< Set underline color (8 or 24 bit color)
        UNDERLINE_DEFAULT = 59, ///< Set default underline color
        FG_BRIGHT_BLACK = 90,
        FG_BRIGHT_RED = 91,
        FG_BRIGHT_GREEN = 92,
        FG_BRIGHT_YELLOW = 93,
        FG_BRIGHT_BLUE = 94,
        FG_BRIGHT_MAGENTA = 95,
        FG_BRIGHT_CYAN = 96,
        FG_BRIGHT_WHITE = 97,
        BG_BRIGHT_BLACK = 100,
        BG_BRIGHT_RED = 101,
        BG_BRIGHT_GREEN = 102,
        BG_BRIGHT_YELLOW = 103,
        BG_BRIGHT_BLUE = 104,
        BG_BRIGHT_MAGENTA = 105,
        BG_BRIGHT_CYAN = 106,
        BG_BRIGHT_WHITE = 107,
    };

    namespace __detail {
        constexpr void process_one_graphics_escape(std::stringstream &ss, bool& first, Graphics type) {
            if (!first)
                ss << ';';
            else
                first = false;

            ss << std::format("{}", static_cast<int>(type));
        }
    }

    /**
     * @brief Create an 8-bit color.
     * @param type Type of color to set
     * @param col 8 Bit color value to set
     * @return Data
     */
    constexpr std::string make_color(Graphics type, std::uint8_t col) {
        return std::format("\033[{};5;{}m", static_cast<int>(type), col);
    }

    /**
     * @brief Create an 16 or 24-bit color.
     * @param type Type of color to set
     * @param r red value
     * @param g green value
     * @param b blue value
     * @return Data
     */
    constexpr std::string make_rgb(Graphics type, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return std::format("\033[{};2;{}m", static_cast<int>(type), r, g, b);
    }

    template <typename... Args>
    std::string make_graphic_escape(Args... args) {
        std::stringstream res;
        bool first = true;
        res << "\033[";

        (__detail::process_one_graphics_escape(res, first, std::forward<Args>(args)), ...);

        res << "m";
        return res.str();
    }
}
