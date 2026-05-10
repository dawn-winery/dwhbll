#include <dwhbll/utils/json.hpp>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/utils/utils.hpp>
#include <dwhbll/utils/string.h>

#include <charconv>
#include <sstream>
#include <string>
#include <utility>
#include <cmath>

namespace dwhbll::json {

json& json::operator[](size_t index) {
    ASSERT(is_array());
    return std::get<json_array>(value).at(index);
}

const json& json::operator[](size_t index) const {
    ASSERT(is_array());
    return std::get<json_array>(value).at(index);
}

// Object key access
json& json::operator[](std::string_view key) {
    ASSERT(is_object());
    return std::get<json_object>(value)[std::string(key)];
}

const json& json::operator[](std::string_view key) const {
    ASSERT(is_object());
    auto& obj = std::get<json_object>(value);
    auto it = obj.find(std::string(key));
    if (it == obj.end())
        throw std::out_of_range("Key not found: " + std::string(key));
    return it->second;
}

std::string json::format_literal() const {
    ASSERT(is_null() || is_string() || is_number() || is_bool());
    if(is_null())
        return "null";
    if(is_string())
        return "\"" + utils::escape_string(as_string()) + "\"";
    if(is_number()) {
        auto val = as_number();
        if (std::isinf(val) || std::isnan(val))
            return "null";
        return std::format("{}", val);
    }
    if(is_bool())
        return as_bool() ? "true" : "false";

    std::unreachable();
}

std::string json::format_internal(int indentation = -1, int cur_indentation = 0) const {
    if(!is_object() && !is_array())
        return format_literal();

    bool pretty = indentation >= 0;

    std::string ind;
    std::string base_ind;
    std::ostringstream ss;

    if(pretty) {
        base_ind = std::string(cur_indentation, ' ');
        cur_indentation += indentation;
        ind = std::string(cur_indentation, ' ');
    }

    if(is_object()) {
        json_object members = as_object();
        ss << "{";
        if(!members.empty() && pretty)
            ss << "\n";

        auto it = members.begin();
        while(it != members.end()) {
            if(pretty)
                ss << ind;
            ss << "\"" << utils::escape_string(it->first) << "\"" 
                << (pretty ? ": " : ":") << it->second.format_internal(indentation, cur_indentation);

            ++it;
            if(it != members.end()) {
                ss << (pretty ? ",\n" : ",");
            }
        }

        if(!members.empty() && pretty)
            ss << "\n" + base_ind;
        ss << "}";
    }
    else if(is_array()) {
        json_array elements = as_array();
        ss << "[";
        if(!elements.empty() && pretty)
            ss << "\n";

        for(size_t i = 0; i < elements.size(); i++) {
            if(pretty)
                ss << ind;
            ss << elements[i].format_internal(indentation, cur_indentation);

            if(i != elements.size() - 1)
                ss << (pretty ? ",\n" : ",");
        }

        if(!elements.empty() && pretty)
            ss << "\n" + base_ind;
        ss << "]";
    }

    return ss.str();
}

std::string json::dump() const {
    return format_internal();
}

std::string json::format(int indentation) const {
    return format_internal(indentation);
}

json json::parse(std::string_view s) {
    auto parser = json_parser(s);
    return parser.parse();
}

json json::parse(std::istream& stream) {
    std::string s(std::istreambuf_iterator<char>(stream), {});
    auto parser = json_parser(s);
    return parser.parse();
}

void json_parser::ws() {
    while(idx < data.size()) {
        char c = data[idx];
        if(c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            idx++;
        } else {
            break;
        }
    }
}

std::pair<std::string, json> json_parser::member() {
    ws();
    std::string id = string();
    ws();
    match(':');
    return { id, element() };
}

json::json_object json_parser::object() {
    match('{');
    json::json_object j;
    ws();
    if(peek() == '}') {
        match('}');
        return j;
    }
    while(1) {
        auto [id, val] = member();
        j[id] = val;
        ws();
        char c = peek();
        if(c == '}') {
            break;
        }
        match(',');
    }
    match('}');
    return j;
}

json::json_array json_parser::array() {
    match('[');
    json::json_array j;
    ws();
    if(peek() == ']') {
        match(']');
        return j;
    }
    while(1) {
        auto val = element();
        j.push_back(val);
        ws();
        char c = peek();
        if(c == ']') {
            break;
        }
        match(',');
    }
    match(']');
    return j;
}

std::string json_parser::string() {
    match('"');
    std::string s;
    while(1) {
        char c = peek();
        if(c == '"')
            break;
        consume();
        if(c == '\\') {
            c = peek();
            consume();
            switch(c) {
                case '"':
                case '\\':
                case '/':
                    s += c;
                    break;
                case 'b':
                    s += '\b';
                    break;
                case 'f':
                    s += '\f';
                    break;
                case 'n':
                    s += '\n';
                    break;
                case 'r':
                    s += '\r';
                    break;
                case 't':
                    s += '\t';
                    break;
                case 'u': {
                    uint32_t cp = 0;
                    for(int i = 0; i < 4; i++) {
                        char h = consume();
                        cp <<= 4;
                        if(h >= '0' && h <= '9') cp |= (h - '0');
                        else if(h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                        else if(h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                        else debug::panic("Error while parsing JSON: invalid hex digit in \\u escape");
                    }
                    if (cp <= 0x7f) {
                        s += static_cast<char>(cp);
                    } else if (cp <= 0x7ff) {
                        s += static_cast<char>(0xc0 | (cp >> 6));
                        s += static_cast<char>(0x80 | (cp & 0x3f));
                    } else if (cp <= 0xffff) {
                        s += static_cast<char>(0xe0 | (cp >> 12));
                        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
                        s += static_cast<char>(0x80 | (cp & 0x3f));
                    }
                    break;
                }
                default:
                    debug::panic("Error while parsing JSON: unexpected character while parsing escape sequence: '{}'", c);
            }
            continue;
        }
        s.push_back(c);
    }
    match('"');
    return s;
}

sanify::f64 json_parser::number() {
    const char* start = data.data() + idx;
    const char* end = data.data() + data.size();
    sanify::f64 num;
    auto [ptr, ec] = std::from_chars(start, end, num);
    if(ec != std::errc())
        debug::panic("Error while parsing JSON: invalid number");

    if (data[idx] == '0' && idx + 1 < data.size() && data[idx+1] >= '0' && data[idx+1] <= '9')
        debug::panic("Error while parsing JSON: leading zeros are not allowed");

    idx += (ptr - start);
    return num;
}

json json_parser::element() {
    ws();
    char c = peek();
    json j;

    if(c == '{')
        j = object();
    else if(c == '[')
        j = array();
    else if(c == '"')
        j = string();
    else if(c == '-' || (c >= '0' && c <= '9'))
        j = number();
    else if(c == 't')  {
        match("true");
        j = true;
    }
    else if(c == 'f') {
        match("false");
        j = false;
    }
    else if(c == 'n') {
        match("null");
        j = nullptr;
    }
    else {
        debug::panic("Error while parsing JSON: unexpected character '{}'", c);
    }

    ws();
    return j;
}

json json_parser::parse() {
    return element();
}

}
