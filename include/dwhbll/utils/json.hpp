#pragma once

#include <dwhbll/sanify/types.hpp>
#include <dwhbll/console/debug.hpp>
#include <dwhbll/utils/utils.hpp>
#include <cassert>
#include <concepts>
#include <map>
#include <utility>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <expected>

namespace dwhbll::json {

class json {

public:
    using json_object = std::map<std::string, json>;
    using json_array = std::vector<json>;

    // Just casually wasting 31 bytes when you want to store a bool
    using value_t = std::variant<
        json_object,
        json_array,
        std::nullptr_t,
        sanify::f64,
        std::string,
        bool
    >;

private:
    value_t value;

    std::string format_literal() const;
    std::string format_internal(int indentation, int cur_indentation) const;

public:
    constexpr json() : value(nullptr) {}
    constexpr json(std::nullptr_t) : value(nullptr) {}

    template <typename T>
    requires std::is_arithmetic_v<T> && (!std::is_same_v<T, bool>)
    constexpr json(T v) : value(static_cast<sanify::f64>(v)) {}

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

    static std::expected<json, std::string> parse(std::string_view s);
    static std::expected<json, std::string> parse(std::istream& s);
};

class json_parser {
private:
    friend class json;

    std::string_view data;
    sanify::u64 idx = 0;

    json_parser(std::string_view v) : data(v) {};

    std::expected<char, std::string> peek();
    std::expected<char, std::string> consume();
    std::expected<void, std::string> match(char c);
    std::expected<void, std::string> match(std::string_view s);

    void ws();
    std::expected<std::pair<std::string, json>, std::string> member();
    std::expected<json::json_object, std::string> object();
    std::expected<json::json_array, std::string> array();
    std::expected<std::string, std::string> string();
    std::expected<sanify::f64, std::string> number();
    std::expected<json, std::string> element();
    std::expected<json, std::string> parse();
};

}
