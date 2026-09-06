#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <dwhbll/stl_ext/option.h>
#include <dwhbll/cli/cli.h>

namespace dwhbll::cli {

    class ArgMatches {
        std::unordered_map<std::string, std::vector<std::string>> values_;
        std::unordered_map<std::string, bool> flags_;
        std::unordered_map<std::string, std::unique_ptr<ArgMatches>> subcommands_;

    public:
        ArgMatches() = default;
        ArgMatches(const ArgMatches&) = delete;
        ArgMatches& operator=(const ArgMatches&) = delete;
        ArgMatches(ArgMatches&&) = default;
        ArgMatches& operator=(ArgMatches&&) = default;

        void insert_value(std::string id, std::string value);
        void insert_flag(std::string id, bool value);
        void insert_subcommand(std::string name, std::unique_ptr<ArgMatches> sub_matches);
        void clear_values(const std::string& id);

        [[nodiscard]] dwhbll::stl_ext::Option<std::string> get_one(const std::string& id) const;
        [[nodiscard]] std::vector<std::string> get_many(const std::string& id) const;
        [[nodiscard]] bool get_flag(const std::string& id) const;

        [[nodiscard]] bool contains_id(const std::string& id) const;
        [[nodiscard]] size_t count(const std::string& id) const;

        [[nodiscard]] dwhbll::stl_ext::Option<std::string> subcommand_name() const;
        [[nodiscard]] const ArgMatches* subcommand_matches(const std::string& name) const;

        template<typename T>
        [[nodiscard]] dwhbll::stl_ext::Option<T> get_one_as(const std::string& id) const;
    };

    struct ParseResult {
        ArgMatches matches;
        std::vector<std::string> errors;
        bool success = true;

        ParseResult() = default;
        ParseResult(ArgMatches m) : matches(std::move(m)) {}
        ParseResult(std::string err) : success(false), errors({std::move(err)}) {}
    };

    class Command {
        std::string name_, about_, version_;
        std::vector<Arg> args_;
        std::unordered_map<std::string, Command> subcommands_;
        bool disable_help_flag_ = false, disable_version_flag_ = false;

    public:
        explicit Command(std::string name) : name_(std::move(name)) {}

        Command& about(std::string a) {
            about_ = std::move(a);
            return *this;
        }

        Command& version(std::string v) {
            version_ = std::move(v);
            return *this;
        }

        Command& arg(Arg arg) {
            args_.push_back(std::move(arg));
            return *this;
        }

        Command& args(std::initializer_list<Arg> args) {
            for (auto& a : args) {
                args_.push_back(std::move(a));
            }
            return *this;
        }

        Command& subcommand(Command cmd) {
            subcommands_.emplace(cmd.name_, std::move(cmd));
            return *this;
        }

        Command& disable_help_flag(bool yes = true) {
            disable_help_flag_ = yes;
            return *this;
        }

        Command& disable_version_flag(bool yes = true) {
            disable_version_flag_ = yes;
            return *this;
        }

        [[nodiscard]] const std::string& name() const noexcept { return name_; }
        [[nodiscard]] const std::string& about() const noexcept { return about_; }
        [[nodiscard]] const std::string& version() const noexcept { return version_; }
        [[nodiscard]] const std::vector<Arg>& args() const noexcept { return args_; }
        [[nodiscard]] const std::unordered_map<std::string, Command>& subcommands() const noexcept { return subcommands_; }

        ParseResult try_get_matches_from(const std::vector<std::string>& argv) const;
        ArgMatches get_matches_from(const std::vector<std::string>& argv) const;
        ArgMatches get_matches(int argc, char** argv) const;

    private:
        ParseResult parse_args(const std::vector<std::string>& argv) const;
        void print_help() const;
        void print_version() const;

        [[nodiscard]] std::string arg_display_name(const Arg& arg) const;
        [[nodiscard]] std::pair<size_t, size_t> get_min_max_values(const Arg& arg) const;
        [[nodiscard]] size_t count_total_values(const Arg& arg, const std::vector<std::string>& raw_values) const;
        void insert_values_with_delimiter(const Arg& arg, ArgMatches& matches, const std::vector<std::string>& raw_values, ArgAction action) const;
        [[nodiscard]] std::vector<std::string> collect_raw_values(const std::vector<std::string>& argv, size_t& i, size_t max_values, const std::string& initial_value) const;
        [[nodiscard]] ParseResult validate_and_insert_values(const Arg& arg, ArgMatches& matches, const std::vector<std::string>& raw_values, const std::string& current) const;
        void insert_default_values(ArgMatches& matches) const;
        void insert_env_values(ArgMatches& matches) const;
        [[nodiscard]] ParseResult check_conflicts(const ArgMatches& matches) const;
        [[nodiscard]] ParseResult check_requires(const ArgMatches& matches) const;
        [[nodiscard]] ParseResult check_num_args_range(const ArgMatches& matches) const;
    };
}
