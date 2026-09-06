#pragma once

#include <string>
#include <vector>
#include <optional>
#include <dwhbll/stl_ext/option.h>
#include <dwhbll/collections/streams.hpp>

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
        dwhbll::stl_ext::Option<ValueHint> value_hint_;
        dwhbll::stl_ext::Option<size_t> index_;
        bool required_ = false, global_ = false, last_ = false, trailing_var_arg_ = false, exclusive_ = false, ignore_case_ = false;
        std::vector<std::string> conflicts_, requires_list_, overrides_, groups_;
        dwhbll::stl_ext::Option<std::string> terminator_, help_heading_, env_, env_default_;

    public:
        explicit Arg(std::string id);

        Arg& id(std::string id);
        [[nodiscard]] const std::string& id() const noexcept { return id_; }

        Arg& short_opt(char s);
        Arg& long_opt(std::string l);
        [[nodiscard]] dwhbll::stl_ext::Option<char> short_opt() const noexcept { return short_opt_; }
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> long_opt() const noexcept { return long_opt_; }

        Arg& help(std::string h);
        Arg& long_help(std::string h);
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> help() const noexcept { return help_; }
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> long_help() const noexcept { return long_help_; }

        Arg& action(ArgAction a);
        [[nodiscard]] dwhbll::stl_ext::Option<ArgAction> action() const noexcept { return action_; }

        Arg& value_name(std::string name);
        Arg& value_names(std::initializer_list<std::string> names);
        [[nodiscard]] const std::vector<std::string>& value_names() const noexcept { return value_names_; }

        Arg& num_args(ValueRange range);
        [[nodiscard]] dwhbll::stl_ext::Option<ValueRange> num_args() const noexcept { return num_args_; }

        Arg& value_delimiter(char c);
        [[nodiscard]] dwhbll::stl_ext::Option<char> value_delimiter() const noexcept { return value_delimiter_; }

        Arg& default_value(std::string val);
        Arg& default_values(std::initializer_list<std::string> vals);
        Arg& default_missing_value(std::string val);
        Arg& default_missing_values(std::initializer_list<std::string> vals);
        [[nodiscard]] const std::vector<std::string>& default_values() const noexcept { return default_values_; }
        [[nodiscard]] const std::vector<std::string>& default_missing_values() const noexcept { return default_missing_values_; }

        Arg& value_hint(ValueHint hint);
        [[nodiscard]] dwhbll::stl_ext::Option<ValueHint> value_hint() const noexcept { return value_hint_; }

        Arg& index(size_t idx);
        [[nodiscard]] dwhbll::stl_ext::Option<size_t> index() const noexcept { return index_; }

        Arg& required(bool yes = true);
        [[nodiscard]] bool required() const noexcept { return required_; }

        Arg& global(bool yes = true);
        [[nodiscard]] bool global() const noexcept { return global_; }

        Arg& last(bool yes = true);
        [[nodiscard]] bool last() const noexcept { return last_; }

        Arg& trailing_var_arg(bool yes = true);
        [[nodiscard]] bool trailing_var_arg() const noexcept { return trailing_var_arg_; }

        Arg& exclusive(bool yes = true);
        [[nodiscard]] bool exclusive() const noexcept { return exclusive_; }

        Arg& ignore_case(bool yes = true);
        [[nodiscard]] bool ignore_case() const noexcept { return ignore_case_; }

        Arg& alias(std::string name, bool visible = false);
        Arg& aliases(std::initializer_list<std::string> names, bool visible = false);
        Arg& short_alias(char name, bool visible = false);
        Arg& short_aliases(std::initializer_list<char> names, bool visible = false);
        [[nodiscard]] const std::vector<std::pair<std::string, bool>>& long_aliases() const noexcept { return long_aliases_; }
        [[nodiscard]] const std::vector<std::pair<char, bool>>& short_aliases() const noexcept { return short_aliases_; }

        Arg& conflicts_with(std::string arg_id);
        Arg& conflicts_with(std::initializer_list<std::string> arg_ids);
        [[nodiscard]] const std::vector<std::string>& conflicts() const noexcept { return conflicts_; }

        Arg& requires_arg(std::string arg_id);
        Arg& requires_arg(std::initializer_list<std::string> arg_ids);
        [[nodiscard]] const std::vector<std::string>& requires_args() const noexcept { return requires_list_; }

        Arg& overrides_with(std::string arg_id);
        Arg& overrides_with(std::initializer_list<std::string> arg_ids);
        [[nodiscard]] const std::vector<std::string>& overrides() const noexcept { return overrides_; }

        Arg& group(std::string group_id);
        Arg& groups(std::initializer_list<std::string> group_ids);
        [[nodiscard]] const std::vector<std::string>& groups() const noexcept { return groups_; }

        Arg& terminator(std::string term);
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> terminator() const noexcept { return terminator_; }

        Arg& help_heading(std::string heading);
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> help_heading() const noexcept { return help_heading_; }

        Arg& env(std::string var_name, dwhbll::stl_ext::Option<std::string> default_val = dwhbll::stl_ext::Option<std::string>());
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> env() const noexcept { return env_; }
        [[nodiscard]] dwhbll::stl_ext::Option<std::string> env_default() const noexcept { return env_default_; }

        bool takes_values() const noexcept;
        bool is_positional() const noexcept;
        bool is_flag() const noexcept;
        bool is_required() const noexcept;
    };

    Arg arg_short_long(char s, std::string l);
}

namespace dwhbll::cli::literals {
    inline Arg operator""_Arg(const char* str, size_t) {
        return Arg(std::string(str));
    }
}
