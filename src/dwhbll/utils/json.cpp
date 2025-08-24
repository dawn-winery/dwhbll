#include <dwhbll/utils/json.hpp>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/utils/utils.hpp>

#include <sstream>
#include <string>
#include <utility>

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
    if(is_number())
        return std::to_string(as_number());
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
        if(!members.empty())
            ss << (pretty ? "\n" : " ");

        auto it = members.begin();
        while(it != members.end()) {
            if(pretty)
                ss << ind;
            ss << "\"" << utils::escape_string(it->first) << "\"" 
                << ": " << it->second.format_internal(indentation, cur_indentation);

            ++it;
            if(it != members.end()) {
                ss << (pretty ? ",\n" : ", ");
            }
        }

        if(!members.empty())
            ss << (pretty ? "\n" + base_ind : " ");
        ss << "}";
    }
    else if(is_array()) {
        json_array elements = as_array();
        ss << "[";
        if(!elements.empty())
            ss << (pretty ? "\n" : " ");

        for(size_t i = 0; i < elements.size(); i++) {
            if(pretty)
                ss << ind;
            ss << elements[i].format_internal(indentation, cur_indentation);

            if(i != elements.size() - 1)
                ss << (pretty ? ",\n" : ", ");
        }

        if(!elements.empty())
            ss << (pretty ? "\n" + base_ind : " ");
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
    if(idx >= data.size())
        return;
    char c = peek();
    while(c == ' ' || c == 0x20 || c == 0xA || c == 0xD || c == 0x9) {
        consume();
        if(idx >= data.size())
            return;
        c = peek();
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
    while(1) {
        ws();
        if(peek() == '}')
            break;
        auto [id, val] = member();
        j[id] = val;
        if(peek() != '}')
            match(',');
    }
    match('}');
    return j;
}

json::json_array json_parser::array() {
    match('[');
    json::json_array j;
    while(1) {
        ws();
        if(peek() == ']')
            break;
        auto val = element();
        j.push_back(val);
        if(peek() != ']')
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
                    s += c;
                    break;
                case 'b':
                    // backspace
                    s += 0x8;
                    break;
                case 'f':
                    // formfeed
                    s += 0xC;
                    break;
                case 'n':
                    // linefeed
                    s += 0xA;
                    break;
                case 'r':
                    // carriage return
                    s += 0xD;
                    break;
                case 't':
                    // horizontal tab
                    s += 0x9;
                    break;
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
    bool negative = false;
    if(peek() == '-') {
        consume();
        negative = true;
    }
    if(peek() < '0' || peek() > '9')
        debug::panic("Error while parsing JSON: unexpected character while parsing number: '{}'", peek());

    sanify::f64 num = 0;
    uint dec = 0;
    while(1) {
        char c = peek();
        if(c < '0' || c > '9')
            break;

        consume();
        if(dec == 0) {
            num *= 10;
            num += c - '0';
        }
        else {
            num += static_cast<sanify::f64>(c - '0') / dec;
            dec *= 10;
        }

        c = peek();
        if(c == '.' && dec == 0) {
            consume();
            dec = 10;
        }
    }

    if(negative)
        return -num;
    else
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
    else {
        match("null");
        j = nullptr;
    }

    ws();
    return j;
}

json json_parser::parse() {
    return element();
}

}
