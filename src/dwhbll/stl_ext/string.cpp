#include <dwhbll/stl_ext/string.h>

#include <format>
#include <ranges>

#include <dwhbll/debug/debug.h>

namespace dwhbll::stl_ext {
    std::string escape_non_printable(const std::string &string) {
        std::string result;
        result.reserve(string.size());

        for (char c : string) {
            if (std::iscntrl(c)) {
                switch (c) {
                    case '\n':
                        result += "\\n";
                        continue;
                    case '\r':
                        result += "\\r";
                        continue;
                    default:
                        break;
                }
            } else if (std::isprint(c)) {
                result += c;
                continue;
            }
            result += "\\x";
            result += std::format("{:X}", c);
        }

        return result;
    }

    constexpr std::vector<std::string> split(std::string_view str, std::string_view sep) {
        std::vector<std::string> strs;
        const std::string::size_type sep_size = sep.size();

        if (sep.empty()) {
            strs.emplace_back(str);
            return strs;
        }

        std::string::size_type start = 0, pos = 0;
        while ((pos = str.find(sep, start)) != std::string::npos) {
            strs.emplace_back(str.substr(start, pos - start));
            start = pos + sep_size;
        }
        strs.emplace_back(str.substr(start));

        return strs;
    }

    std::string escape_string(std::string_view str) {
        std::string s;
        s.reserve(str.size());
        for (char c : str) {
            switch (c) {
                case '\"': s += "\\\""; break;
                case '\\': s += "\\\\"; break;
                case '\b': s += "\\b"; break;
                case '\f': s += "\\f"; break;
                case '\n': s += "\\n"; break;
                case '\r': s += "\\r"; break;
                case '\t': s += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        s += std::format("\\u{:04x}", static_cast<unsigned char>(c));
                    } else {
                        s += c;
                    }
                    break;
            }
        }
        return s;
    }

    std::string utf8_encode(std::u32string_view str) {
        std::string result;

        for (char32_t c : str) {
            if (c < 0x80)
                result += (char)c;
            else if (c < 0x800) {
                result += (char)(((c >> 6) & 0x1F) | 0xC0);
                result += (char)((c & 0x3F) | 0x80);
            } else if (c < 0x10000) {
                result += (char)(((c >> 12) & 0x0F) | 0xE0);
                result += (char)(((c >> 6) & 0x3F) | 0x80);
                result += (char)((c & 0x3F) | 0x80);
            } else if (c < 0x110000) {
                result += (char)(((c >> 18) & 0x07) | 0xF0);
                result += (char)(((c >> 12) & 0x3F) | 0x80);
                result += (char)(((c >> 6) & 0x3F) | 0x80);
                result += (char)((c & 0x3F) | 0x80);
            } else
                debug::panic("Failed to UTF8 encode unicode code pointe U+{:x}", (uint32_t)c);
        }

        return result;
    }
}
