#pragma once

#include <dwhbll/sanify/types.hpp>
#include <dwhbll/console/debug.hpp>
#include <cassert>
#include <map>
#include <utility>
#include <string>
#include <variant>
#include <vector>

namespace dwhbll::json {

class json {

public:
    using json_object = std::map<std::string, json>;
    using json_array = std::vector<json>;

    // Just casually wasting 31 bytes when you want to store a bool
    using value_t = std::variant<
        json_object,
        json_array,
        nullptr_t,
        sanify::f64,
        std::string,
        bool
    >;

private:
    value_t value;

    std::string format_literal() const;
    std::string format_internal(int indentation, int cur_indentation) const;

public:
    constexpr json() : value(json_object()) {}
    constexpr json(nullptr_t) : value(nullptr) {}
    constexpr json(sanify::f64 v) : value(v) {}
    constexpr json(bool v) : value(v) {}
    constexpr json(const std::string& v) : value(v) {}
    constexpr json(std::string&& v) : value(std::move(v)) {}
    constexpr json(const char* v) : value(std::string(v)) {}
    constexpr json(const json_object& v) : value(v) {}
    constexpr json(json_object&& v) : value(std::move(v)) {}
    constexpr json(const json_array& v) : value(v) {}
    constexpr json(json_array&& v) : value(std::move(v)) {}

    constexpr bool is_null() const { return std::holds_alternative<std::nullptr_t>(value); }
    constexpr bool is_number() const { return std::holds_alternative<sanify::f64>(value); }
    constexpr bool is_string() const { return std::holds_alternative<std::string>(value); }
    constexpr bool is_bool() const { return std::holds_alternative<bool>(value); }
    constexpr bool is_object() const { return std::holds_alternative<json_object>(value); }
    constexpr bool is_array() const { return std::holds_alternative<json_array>(value); }

    constexpr const json_object& as_object() const { return std::get<json_object>(value); }
    constexpr json_object& as_object() { return std::get<json_object>(value); }

    constexpr const json_array& as_array() const { return std::get<json_array>(value); }
    constexpr json_array& as_array() { return std::get<json_array>(value); }

    constexpr const std::string& as_string() const { return std::get<std::string>(value); }
    constexpr sanify::f64 as_number() const { return std::get<sanify::f64>(value); }
    constexpr bool as_bool() const { return std::get<bool>(value); }

    json& operator[](size_t index);
    const json& operator[](size_t index) const;

    json& operator[](std::string_view key);
    const json& operator[](std::string_view key) const;

    std::string dump() const;
    std::string format(int indentation = 2) const;

    static json parse(std::string_view s);
    static json parse(std::istream& s);
};

class json_parser {
private:
    friend class json;

    std::string_view data;
    sanify::u64 idx = 0;

    json_parser(std::string_view v) : data(v) {};

    char peek() { 
        if(idx >= data.size())
            debug::panic("Error while parsing JSON: unexpected EOF");
        return data[idx];
    };

    char consume() {
        if(idx >= data.size())
            debug::panic("Error while parsing JSON: unexpected EOF");
        return data[idx++];
    };

    void match(char c) {
        char n = consume();
        if(n != c)
            debug::panic("Error while parsing JSON: unexpected character\nExpected: '{}'\n Found: '{}'", c, n);
    };

    void match(std::string_view s) {
        for(auto c : s)
            match(c);
    }

    void ws();
    std::pair<std::string, json> member();
    json::json_object object();
    json::json_array array();
    std::string string();
    sanify::f64 number();
    json element();
    json parse();
};

}
