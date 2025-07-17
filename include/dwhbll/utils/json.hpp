#pragma once

#include "dwhbll/sanify/types.hpp"
#include <cassert>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace dwhbll::json {

// Can be an object, an array, a string, a number, a null object or a boolean
class json {

public:
    using json_object = std::map<std::string_view, json>;
    using json_array = std::vector<json>;

    // Just casually wasting 31 bytes when you want to store a bool
    using value_t = std::variant<
        json_object,
        json_array,
        nullptr_t,
        sanify::i64,
        sanify::f64,
        std::string,
        bool
    >;

private:
    value_t value;

    std::string format_internal(int indentation, int cur_indentation) const;

public:
    json() : value(json_object()) {}
    json(nullptr_t) : value(nullptr) {}
    json(sanify::i64 v) : value(v) {}
    json(sanify::f64 v) : value(v) {}
    json(const std::string& v) : value(v) {}
    json(std::string&& v) : value(std::move(v)) {}
    json(const char* v) : value(std::string(v)) {}
    json(bool v) : value(v) {}
    json(const json_object& v) : value(v) {}
    json(json_object&& v) : value(std::move(v)) {}
    json(const json_array& v) : value(v) {}
    json(json_array&& v) : value(std::move(v)) {}

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value); }
    bool is_integer() const { return std::holds_alternative<sanify::i64>(value); }
    bool is_float() const { return std::holds_alternative<sanify::f64>(value); }
    bool is_string() const { return std::holds_alternative<std::string>(value); }
    bool is_bool() const { return std::holds_alternative<bool>(value); }
    bool is_object() const { return std::holds_alternative<json_object>(value); }
    bool is_array() const { return std::holds_alternative<json_array>(value); }

    const json_object& as_object() const { return std::get<json_object>(value); }
    json_object& as_object() { return std::get<json_object>(value); }

    const json_array& as_array() const { return std::get<json_array>(value); }
    json_array& as_array() { return std::get<json_array>(value); }

    const std::string& as_string() const { return std::get<std::string>(value); }
    sanify::i64 as_integer() const { return std::get<sanify::i64>(value); }
    sanify::f64 as_float() const { return std::get<sanify::f64>(value); }
    bool as_bool() const { return std::get<bool>(value); }

    json& operator[](size_t index);
    const json& operator[](size_t index) const;

    json& operator[](std::string_view key);
    const json& operator[](std::string_view key) const;

    std::string dump() const;
    std::string format(int indentation = 2) const;
};

}
