#pragma once

#include <string>
#include <vector>
#include <optional>
#include <dwhbll/stl_ext/option.h>

namespace dwhbll::cli {

enum class ArgAction {
    Set,
    Append,
    Count,
    SetTrue,
    SetFalse,
    Help,
    Version,
    HelpLong,
    VersionLong,
};

enum class ValueHint {
    Unknown,
    Other,
    Path,
    FilePath,
    DirPath,
    ExecutablePath,
    CommandString,
    CommandWithArguments,
    Username,
    Hostname,
    Url,
    Email,
    AnyPath,
    FilePaths,
    DirPaths,
};

class ValueParser {
public:
    virtual ~ValueParser() = default;
    virtual bool parse(const std::string& input, std::string& output) const = 0;
    virtual std::string type_name() const = 0;
    virtual std::unique_ptr<ValueParser> clone() const = 0;
};

class StringValueParser : public ValueParser {
public:
    bool parse(const std::string& input, std::string& output) const override;
    std::string type_name() const override;
    std::unique_ptr<ValueParser> clone() const override;
};

class IntValueParser : public ValueParser {
public:
    bool parse(const std::string& input, std::string& output) const override;
    std::string type_name() const override;
    std::unique_ptr<ValueParser> clone() const override;
};

class UIntValueParser : public ValueParser {
public:
    bool parse(const std::string& input, std::string& output) const override;
    std::string type_name() const override;
    std::unique_ptr<ValueParser> clone() const override;
};

class FloatValueParser : public ValueParser {
public:
    bool parse(const std::string& input, std::string& output) const override;
    std::string type_name() const override;
    std::unique_ptr<ValueParser> clone() const override;
};

class BoolValueParser : public ValueParser {
public:
    bool parse(const std::string& input, std::string& output) const override;
    std::string type_name() const override;
    std::unique_ptr<ValueParser> clone() const override;
};

class PossibleValuesParser : public ValueParser {
    std::vector<std::string> values_;
public:
    PossibleValuesParser(std::initializer_list<std::string> vals);
    PossibleValuesParser(const std::vector<std::string>& vals);
    bool parse(const std::string& input, std::string& output) const override;
    std::string type_name() const override;
    std::unique_ptr<ValueParser> clone() const override;
};

// factory functions for common value parsers
inline std::unique_ptr<ValueParser> value_parser_string() {
    return std::make_unique<StringValueParser>();
}

inline std::unique_ptr<ValueParser> value_parser_int() {
    return std::make_unique<IntValueParser>();
}

inline std::unique_ptr<ValueParser> value_parser_uint() {
    return std::make_unique<UIntValueParser>();
}

inline std::unique_ptr<ValueParser> value_parser_float() {
    return std::make_unique<FloatValueParser>();
}

inline std::unique_ptr<ValueParser> value_parser_bool() {
    return std::make_unique<BoolValueParser>();
}

inline std::unique_ptr<ValueParser> value_parser(std::initializer_list<std::string> vals) {
    return std::make_unique<PossibleValuesParser>(vals);
}

namespace detail {
    inline std::unique_ptr<ValueParser> make_value_parser(std::initializer_list<std::string> vals) {
        return value_parser(vals);
    }
}

struct ValueRange {
    std::optional<size_t> min, max;

    static ValueRange fixed(size_t n) {
        return ValueRange{n, n};
    }
    static ValueRange at_least(size_t n) {
        return ValueRange{n, std::nullopt};
    }
    static ValueRange at_most(size_t n) {
        return ValueRange{0, n};
    }
    static ValueRange range(size_t min, size_t max) {
        return ValueRange{min, max};
    }
    static ValueRange any() {
        return ValueRange{0, std::nullopt};
    }
};

enum class ArgPredicateType {
    IsPresent,
    Equals,
    NotEquals,
};

struct ArgPredicate {
    ArgPredicateType type;
    std::string arg_id;
    std::vector<std::string> values;

    static ArgPredicate is_present(std::string arg_id) {
        return ArgPredicate{ArgPredicateType::IsPresent, std::move(arg_id), {}};
    }
    static ArgPredicate equals(std::string arg_id, std::string value) {
        return ArgPredicate{ArgPredicateType::Equals, std::move(arg_id), {std::move(value)}};
    }
    static ArgPredicate equals_any(std::string arg_id, std::initializer_list<std::string> vals) {
        return ArgPredicate{ArgPredicateType::Equals, std::move(arg_id), {vals}};
    }
    static ArgPredicate not_equals(std::string arg_id, std::string value) {
        return ArgPredicate{ArgPredicateType::NotEquals, std::move(arg_id), {std::move(value)}};
    }
};

struct ConditionalDefault {
    ArgPredicate predicate;
    std::vector<std::string> values;
    bool is_if_all = false;
    bool is_unless = false;
};

class Arg {
    std::string id_;
    dwhbll::stl_ext::Option<std::string> help_, long_help_;
    dwhbll::stl_ext::Option<char> short_opt_;
    dwhbll::stl_ext::Option<std::string> long_opt_;
    std::vector<std::pair<std::string, bool>> long_aliases_;
    std::vector<std::pair<char, bool>> short_aliases_;
    dwhbll::stl_ext::Option<ArgAction> action_;
    std::vector<std::string> value_names_;
    dwhbll::stl_ext::Option<ValueRange> num_args_;
    dwhbll::stl_ext::Option<char> value_delimiter_;
    std::vector<std::string> default_values_, default_missing_values_;
    std::vector<ConditionalDefault> default_value_ifs_;
    dwhbll::stl_ext::Option<ValueHint> value_hint_;
    dwhbll::stl_ext::Option<size_t> index_;
    bool required_ = false, global_ = false, last_ = false, trailing_var_arg_ = false, exclusive_ = false, ignore_case_ = false;
    bool allow_hyphen_values_ = false, allow_negative_numbers_ = false, require_equals_ = false, raw_ = false;
    dwhbll::stl_ext::Option<size_t> display_order_;
    bool next_line_help_ = false, hide_short_help_ = false, hide_long_help_ = false, hide_possible_values_ = false, hide_default_value_ = false, hide_env_ = false, hide_env_values_ = false;
    std::vector<std::string> conflicts_, requires_list_, overrides_, groups_;
    dwhbll::stl_ext::Option<std::string> terminator_, help_heading_, env_, env_default_, value_terminator_;
    std::unique_ptr<ValueParser> value_parser_;

public:
    explicit Arg(std::string id);
    Arg(const Arg &other);
    Arg &operator=(const Arg &other);
    Arg(Arg&&) = default;
    Arg &operator=(Arg&&) = default;

    Arg &id(std::string id);
    [[nodiscard]] const std::string &id() const noexcept { return id_; }

    Arg &short_opt(char s);
    Arg &long_opt(std::string l);
    [[nodiscard]] dwhbll::stl_ext::Option<char> short_opt() const noexcept { return short_opt_; }
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> long_opt() const noexcept { return long_opt_; }

    Arg &help(std::string h);
    Arg &long_help(std::string h);
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> help() const noexcept { return help_; }
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> long_help() const noexcept { return long_help_; }

    Arg &action(ArgAction a);
    [[nodiscard]] dwhbll::stl_ext::Option<ArgAction> action() const noexcept { return action_; }

    Arg &value_name(std::string name);
    Arg& value_names(std::initializer_list<std::string> names);
    [[nodiscard]] const std::vector<std::string> &value_names() const noexcept { return value_names_; }

    Arg& num_args(ValueRange range);
    [[nodiscard]] dwhbll::stl_ext::Option<ValueRange> num_args() const noexcept { return num_args_; }

    Arg &value_delimiter(char c);
    [[nodiscard]] dwhbll::stl_ext::Option<char> value_delimiter() const noexcept { return value_delimiter_; }

    Arg &default_value(std::string val);
    Arg &default_values(std::initializer_list<std::string> vals);
    Arg& default_missing_value(std::string val);
    Arg &default_missing_values(std::initializer_list<std::string> vals);
    [[nodiscard]] const std::vector<std::string> &default_values() const noexcept { return default_values_; }
    [[nodiscard]] const std::vector<std::string> &default_missing_values() const noexcept { return default_missing_values_; }

    Arg &default_value_if(ArgPredicate pred, std::string val);
    Arg &default_value_if_all(ArgPredicate pred, std::initializer_list<std::string> vals);
    Arg &default_value_unless(ArgPredicate pred, std::string val);

    Arg &value_hint(ValueHint hint);
    [[nodiscard]] dwhbll::stl_ext::Option<ValueHint> value_hint() const noexcept { return value_hint_; }

    Arg &value_parser(std::unique_ptr<ValueParser> parser);
    Arg &value_parser(std::initializer_list<std::string> vals);
    [[nodiscard]] const ValueParser* value_parser() const noexcept { return value_parser_.get(); }

    Arg &index(size_t idx);
    [[nodiscard]] dwhbll::stl_ext::Option<size_t> index() const noexcept { return index_; }

    Arg &required(bool yes = true);
    [[nodiscard]] bool required() const noexcept { return required_; }

    Arg &global(bool yes = true);
    [[nodiscard]] bool global() const noexcept { return global_; }

    Arg &last(bool yes = true);
    [[nodiscard]] bool last() const noexcept { return last_; }

    Arg &trailing_var_arg(bool yes = true);
    [[nodiscard]] bool trailing_var_arg() const noexcept { return trailing_var_arg_; }

    Arg &exclusive(bool yes = true);
    [[nodiscard]] bool exclusive() const noexcept { return exclusive_; }

    Arg &ignore_case(bool yes = true);
    [[nodiscard]] bool ignore_case() const noexcept { return ignore_case_; }

    Arg &allow_hyphen_values(bool yes = true);
    [[nodiscard]] bool allow_hyphen_values() const noexcept { return allow_hyphen_values_; }

    Arg &allow_negative_numbers(bool yes = true);
    [[nodiscard]] bool allow_negative_numbers() const noexcept { return allow_negative_numbers_; }

    Arg &require_equals(bool yes = true);
    [[nodiscard]] bool require_equals() const noexcept { return require_equals_; }

    Arg &value_terminator(std::string term);
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> value_terminator() const noexcept { return value_terminator_; }

    Arg &raw(bool yes = true);
    [[nodiscard]] bool raw() const noexcept { return raw_; }

    Arg &display_order(size_t order);
    [[nodiscard]] dwhbll::stl_ext::Option<size_t> display_order() const noexcept { return display_order_; }

    Arg &next_line_help(bool yes = true);
    [[nodiscard]] bool next_line_help() const noexcept { return next_line_help_; }

    Arg &hide_short_help(bool yes = true);
    [[nodiscard]] bool hide_short_help() const noexcept { return hide_short_help_; }

    Arg &hide_long_help(bool yes = true);
    [[nodiscard]] bool hide_long_help() const noexcept { return hide_long_help_; }

    Arg &hide_possible_values(bool yes = true);
    [[nodiscard]] bool hide_possible_values() const noexcept { return hide_possible_values_; }

    Arg &hide_default_value(bool yes = true);
    [[nodiscard]] bool hide_default_value() const noexcept { return hide_default_value_; }

    Arg &hide_env(bool yes = true);
    [[nodiscard]] bool hide_env() const noexcept { return hide_env_; }

    Arg &hide_env_values(bool yes = true);
    [[nodiscard]] bool hide_env_values() const noexcept { return hide_env_values_; }

    Arg &alias(std::string name, bool visible = false);
    Arg &aliases(std::initializer_list<std::string> names, bool visible = false);
    Arg &short_alias(char name, bool visible = false);
    Arg& short_aliases(std::initializer_list<char> names, bool visible = false);
    [[nodiscard]] const std::vector<std::pair<std::string, bool>> &long_aliases() const noexcept { return long_aliases_; }
    [[nodiscard]] const std::vector<std::pair<char, bool>> &short_aliases() const noexcept { return short_aliases_; }

    Arg &conflicts_with(std::string arg_id);
    Arg &conflicts_with(std::initializer_list<std::string> arg_ids);
    [[nodiscard]] const std::vector<std::string> &conflicts() const noexcept { return conflicts_; }

    Arg &requires_arg(std::string arg_id);
    Arg &requires_arg(std::initializer_list<std::string> arg_ids);
    [[nodiscard]] const std::vector<std::string> &requires_args() const noexcept { return requires_list_; }

    Arg &overrides_with(std::string arg_id);
    Arg &overrides_with(std::initializer_list<std::string> arg_ids);
    [[nodiscard]] const std::vector<std::string> &overrides() const noexcept { return overrides_; }

    Arg &group(std::string group_id);
    Arg &groups(std::initializer_list<std::string> group_ids);
    [[nodiscard]] const std::vector<std::string> &groups() const noexcept { return groups_; }

    Arg &terminator(std::string term);
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> terminator() const noexcept { return terminator_; }

    Arg &help_heading(std::string heading);
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> help_heading() const noexcept { return help_heading_; }

    Arg &env(std::string var_name, dwhbll::stl_ext::Option<std::string> default_val = dwhbll::stl_ext::Option<std::string>());
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> env() const noexcept { return env_; }
    [[nodiscard]] dwhbll::stl_ext::Option<std::string> env_default() const noexcept { return env_default_; }

    bool takes_values() const noexcept;
    bool is_positional() const noexcept;
    bool is_flag() const noexcept;
    bool is_required() const noexcept;
    [[nodiscard]] const std::vector<ConditionalDefault> &default_value_ifs() const noexcept { return default_value_ifs_; }
};

class ArgGroup {
    std::string id_;
    std::vector<std::string> args_;
    bool required_ = false;
    bool multiple_ = false;
    std::vector<std::string> requires_;
    std::vector<std::string> conflicts_;

public:
    explicit ArgGroup(std::string id);

    ArgGroup &id(std::string id);
    [[nodiscard]] const std::string &id() const noexcept { return id_; }

    ArgGroup &arg(std::string arg_id);
    ArgGroup &args(std::initializer_list<std::string> arg_ids);
    [[nodiscard]] const std::vector<std::string> &args() const noexcept { return args_; }

    ArgGroup& multiple(bool yes = true);
    [[nodiscard]] bool multiple() const noexcept { return multiple_; }

    ArgGroup &required(bool yes = true);
    [[nodiscard]] bool required() const noexcept { return required_; }

    ArgGroup &requires_arg(std::string arg_id);
    ArgGroup &requires_args(std::initializer_list<std::string> arg_ids);
    [[nodiscard]] const std::vector<std::string> &requires_args() const noexcept { return requires_; }

    ArgGroup &conflicts_with(std::string arg_id);
    ArgGroup &conflicts_with_all(std::initializer_list<std::string> arg_ids);
    [[nodiscard]] const std::vector<std::string> &conflicts() const noexcept { return conflicts_; }
};

Arg arg_short_long(char s, std::string l);
}

namespace dwhbll::cli::literals {

inline Arg operator""_Arg(const char* str, size_t) {
    return Arg(std::string(str));
}

}
