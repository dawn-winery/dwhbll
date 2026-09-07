#include <dwhbll/cli/command.h>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <sstream>

extern int __argc;
extern char** __argv;

namespace dwhbll::cli {

Command &Command::about(std::string a) {
    about_ = std::move(a);
    return *this;
}

Command &Command::version(std::string v) {
    version_ = std::move(v);
    return *this;
}

Command &Command::arg(Arg arg) {
    args_.push_back(std::move(arg));
    return *this;
}

Command &Command::args(std::vector<Arg> args) {
    for (auto& a : args) {
        args_.push_back(std::move(a));
    }
    return *this;
}

Command &Command::subcommand(Command cmd) {
    subcommands_.emplace(cmd.name_, std::move(cmd));
    return *this;
}

Command &Command::mut_arg(std::string arg_id, std::function<Arg(Arg)> f) {
    for (auto& arg : args_) {
        if (arg.id() == arg_id) {
            arg = f(std::move(arg));
            break;
        }
    }
    return *this;
}

Command &Command::mut_args(std::function<Arg(Arg)> f) {
    for (auto& arg : args_) {
        arg = f(std::move(arg));
    }
    return *this;
}

Command &Command::mut_group(std::string group_id, std::function<ArgGroup(ArgGroup)> f) {
    for (auto& group : groups_) {
        if (group.id() == group_id) {
            group = f(std::move(group));
            break;
        }
    }
    return *this;
}

Command &Command::mut_subcommand(std::string name, std::function<Command(Command)> f) {
    auto it = subcommands_.find(name);
    if (it != subcommands_.end()) {
        Command subcmd = std::move(it->second);
        subcommands_.erase(it);
        subcommands_.emplace(name, f(std::move(subcmd)));
    }
    return *this;
}

Command &Command::mut_subcommands(std::function<Command(Command)> f) {
    std::unordered_map<std::string, Command> new_subcommands;
    for (auto& [name, subcmd] : subcommands_) {
        new_subcommands.emplace(name, f(std::move(subcmd)));
    }
    subcommands_ = std::move(new_subcommands);
    return *this;
}

Command &Command::alias(std::string name) {
    aliases_.push_back(std::move(name));
    return *this;
}

Command &Command::aliases(std::initializer_list<std::string> names) {
    for (auto& n : names) {
        aliases_.push_back(std::move(n));
    }
    return *this;
}

Command &Command::disable_help_flag(bool yes) {
    disable_help_flag_ = yes;
    return *this;
}

Command &Command::disable_version_flag(bool yes) {
    disable_version_flag_ = yes;
    return *this;
}

Command &Command::group(ArgGroup group) {
    groups_.push_back(std::move(group));
    return *this;
}

Command &Command::groups(std::initializer_list<ArgGroup> grps) {
    for (auto& g : grps) {
        groups_.push_back(std::move(g));
    }
    return *this;
}

Command &Command::no_binary_name(bool yes) {
    settings_[CommandSetting::NoBinaryName] = yes;
    return *this;
}

Command &Command::ignore_errors(bool yes) {
    settings_[CommandSetting::IgnoreErrors] = yes;
    return *this;
}

Command &Command::args_override_self(bool yes) {
    settings_[CommandSetting::ArgsOverrideSelf] = yes;
    return *this;
}

Command &Command::dont_delimit_trailing_values(bool yes) {
    settings_[CommandSetting::DontDelimitTrailingValues] = yes;
    return *this;
}

Command &Command::external_subcommand(bool yes) {
    external_subcommand_ = yes;
    return *this;
}

Command &Command::propagate_version(bool yes) {
    settings_[CommandSetting::PropagateVersion] = yes;
    return *this;
}

Command &Command::next_line_help(bool yes) {
    settings_[CommandSetting::NextLineHelp] = yes;
    return *this;
}

[[nodiscard]] bool Command::get_setting(CommandSetting setting) const noexcept {
    auto it = settings_.find(setting);
    return it != settings_.end() && it->second;
}

ParseResult Command::try_get_matches_from(const std::vector<std::string>& argv) const {
    return parse_args(argv);
}

ArgMatches Command::get_matches_from(const std::vector<std::string>& argv) const {
    auto result = try_get_matches_from(argv);
    if (!result.success) {
        for (const auto& err : result.errors) {
            if (err == "DisplayHelp") {
                print_help();
                std::exit(0);
            } else if (err == "DisplayVersion") {
                print_version();
                std::exit(0);
            } else {
                std::cerr << err << "\n";
            }
        }
        std::exit(1);
    }
    return std::move(result.matches);
}

ArgMatches Command::get_matches(int argc, char** argv) const {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return get_matches_from(args);
}

ParseResult Command::parse_args(const std::vector<std::string>& argv) const {
    ArgMatches matches;
    std::vector<std::string> positional_args;

    std::unordered_map<char, const Arg*> short_map;
    std::unordered_map<std::string, const Arg*> long_map;

    for (const auto& arg : args_) {
        if (arg.short_opt().is_some()) {
            short_map[arg.short_opt().unwrap()] = &arg;
        }
        if (arg.long_opt().is_some()) {
            long_map[arg.long_opt().unwrap()] = &arg;
        }
        for (const auto& alias : arg.long_aliases()) {
            long_map[alias.first] = &arg;
        }
        for (const auto& alias : arg.short_aliases()) {
            short_map[alias.first] = &arg;
        }
    }

    if (!disable_help_flag_) {
        static Arg help_arg = Arg("help")
            .short_opt('h')
            .long_opt("help")
            .help("Print help")
            .action(ArgAction::Help);
        short_map['h'] = &help_arg;
        long_map["help"] = &help_arg;
    }

    if (!disable_version_flag_) {
        static Arg version_arg = Arg("version")
            .short_opt('V')
            .long_opt("version")
            .help("Print version")
            .action(ArgAction::Version);
        short_map['V'] = &version_arg;
        long_map["version"] = &version_arg;
    }

    size_t i = 0;
    while (i < argv.size()) {
        const std::string& current = argv[i];

        if (current == "--") {
            for (size_t j = i + 1; j < argv.size(); ++j) {
                positional_args.push_back(argv[j]);
            }
            break;
        }

        if (current.size() > 1 && current[0] == '-') {
            if (current[1] == '-') {
                std::string long_name = current.substr(2);
                std::string value;
                size_t eq_pos = long_name.find('=');
                if (eq_pos != std::string::npos) {
                    value = long_name.substr(eq_pos + 1);
                    long_name = long_name.substr(0, eq_pos);
                }

                auto it = long_map.find(long_name);
                if (it == long_map.end()) {
                    if (get_setting(CommandSetting::IgnoreErrors)) {
                        // Skip this argument and continue
                    } else {
                        return ParseResult("error: unknown argument '" + current + "'");
                    }
                }

                const Arg* arg = it->second;
                if (arg->require_equals() && value.empty()) {
                    if (get_setting(CommandSetting::IgnoreErrors)) {
                        // Skip this argument and continue
                    } else {
                        return ParseResult("error: argument '" + current + "' requires a value (use --option=value)");
                    }
                }

                if (arg->action().is_some()) {
                    auto action = arg->action().unwrap();
                    if (action == ArgAction::SetTrue || action == ArgAction::SetFalse ||
                        action == ArgAction::Count || action == ArgAction::Help ||
                        action == ArgAction::Version) {
                        if (!value.empty()) {
                            if (get_setting(CommandSetting::IgnoreErrors)) {
                                // Skip this argument and continue
                            } else {
                                return ParseResult("error: flag '" + current + "' doesn't take a value");
                            }
                        }
                        if (action == ArgAction::Count) {
                            matches.insert_value(arg->id(), "1");
                        } else if (action == ArgAction::Help || action == ArgAction::Version) {
                            matches.insert_flag(arg->id(), true);
                        } else {
                            matches.insert_flag(arg->id(), action == ArgAction::SetTrue);
                        }
                    } else {
                        auto action = arg->action().is_some() ? arg->action().unwrap() : ArgAction::Set;
                        auto [min_values, max_values] = get_min_max_values(*arg);
                        auto raw_values = collect_raw_values(argv, i, max_values, value, *arg);
                        auto result = validate_and_insert_values(*arg, matches, raw_values, current);
                        if (!result.success) return result;
                        matches = std::move(result.matches);
                    }
                } else {
                    auto action = arg->action().is_some() ? arg->action().unwrap() : ArgAction::Set;
                    auto [min_values, max_values] = get_min_max_values(*arg);
                    std::string value;
                    if (i + 1 >= argv.size()) {
                        if (get_setting(CommandSetting::IgnoreErrors)) {
                            // Skip this argument and continue
                        } else {
                            return ParseResult("error: argument '" + current + "' requires a value");
                        }
                    } else {
                        value = argv[++i];
                        auto raw_values = collect_raw_values(argv, i, max_values, value, *arg);
                        auto result = validate_and_insert_values(*arg, matches, raw_values, current);
                        if (!result.success) return result;
                        matches = std::move(result.matches);
                    }
                }
            } else {
                std::string short_flags = current.substr(1);
                for (size_t j = 0; j < short_flags.size(); ++j) {
                    char flag = short_flags[j];
                    auto it = short_map.find(flag);
                    if (it == short_map.end()) {
                        if (get_setting(CommandSetting::IgnoreErrors)) {
                            // Skip this argument and continue
                        } else {
                            return ParseResult("error: unknown argument '-" + std::string(1, flag) + "'");
                        }
                    }

                    const Arg* arg = it->second;
                    if (arg->action().is_some()) {
                        auto action = arg->action().unwrap();
                        if (action == ArgAction::SetTrue || action == ArgAction::SetFalse ||
                            action == ArgAction::Count || action == ArgAction::Help ||
                            action == ArgAction::Version) {
                            if (action == ArgAction::Count) {
                                matches.insert_value(arg->id(), "1");
                            } else if (action == ArgAction::Help || action == ArgAction::Version) {
                                matches.insert_flag(arg->id(), true);
                            } else {
                                matches.insert_flag(arg->id(), action == ArgAction::SetTrue);
                            }
                        } else {
                            auto action = arg->action().is_some() ? arg->action().unwrap() : ArgAction::Set;
                            auto [min_values, max_values] = get_min_max_values(*arg);

                            std::string value;
                            if (j + 1 < short_flags.size()) {
                                value = short_flags.substr(j + 1);
                                j = short_flags.size();
                            } else if (i + 1 < argv.size()) {
                                value = argv[++i];
                            } else {
                                if (get_setting(CommandSetting::IgnoreErrors)) {
                                    // Skip this argument and continue
                                } else {
                                    return ParseResult("error: argument '-" + std::string(1, flag) + "' requires a value");
                                }
                            }

                            if (!value.empty()) {
                                auto raw_values = collect_raw_values(argv, i, max_values, value, *arg);
                                auto result = validate_and_insert_values(*arg, matches, raw_values, "-" + std::string(1, flag));
                                if (!result.success) return result;
                                matches = std::move(result.matches);
                            }
                        }
                    } else {
                        std::string value;
                        if (j + 1 < short_flags.size()) {
                            value = short_flags.substr(j + 1);
                            j = short_flags.size();
                        } else if (i + 1 < argv.size()) {
                            value = argv[++i];
                        } else {
                            if (get_setting(CommandSetting::IgnoreErrors)) {
                                // Skip this argument and continue
                            } else {
                                return ParseResult("error: argument '-" + std::string(1, flag) + "' requires a value");
                            }
                        }
                        if (!value.empty()) {
                            matches.insert_value(arg->id(), value);
                        }
                    }
                }
            }
        } else {
            auto sub_it = subcommands_.end();
            if (!subcommands_.empty()) {
                sub_it = subcommands_.find(current);
                if (sub_it == subcommands_.end()) {
                    for (const auto& [name, subcmd] : subcommands_) {
                        for (const auto& alias : subcmd.aliases_) {
                            if (alias == current) {
                                sub_it = subcommands_.find(name);
                                break;
                            }
                        }
                        if (sub_it != subcommands_.end()) break;
                    }
                }
            }
            if (sub_it != subcommands_.end()) {
                std::vector<std::string> sub_argv(argv.begin() + i + 1, argv.end());
                auto sub_result = sub_it->second.try_get_matches_from(sub_argv);
                if (!sub_result.success) {
                    return sub_result;
                }
                matches.insert_subcommand(sub_it->first, std::make_unique<ArgMatches>(std::move(sub_result.matches)));
                positional_args.push_back(current);
                break;
            } else if (external_subcommand_) {
                matches.insert_value("external_subcommand", current);
                std::vector<std::string> ext_args(argv.begin() + i + 1, argv.end());
                for (size_t j = 0; j < ext_args.size(); ++j) {
                    matches.insert_value("external_subcommand_arg", ext_args[j]);
                }
                break;
            }
            positional_args.push_back(current);
        }
        ++i;
    }

    size_t pos_idx = 1;
    for (const auto& arg : args_) {
        if (arg.is_positional()) {
            if (arg.index().is_some()) {
                pos_idx = arg.index().unwrap();
            }
            if (arg.trailing_var_arg()) {
                for (size_t j = pos_idx - 1; j < positional_args.size(); ++j) {
                    matches.insert_value(arg.id(), positional_args[j]);
                }
                pos_idx = positional_args.size() + 1;
            } else if (pos_idx <= positional_args.size()) {
                matches.insert_value(arg.id(), positional_args[pos_idx - 1]);
                ++pos_idx;
            } else if (arg.required()) {
                return ParseResult("error: required positional argument '" + arg.id() + "' not provided");
            } else if (!arg.default_values().empty()) {
                matches.insert_value(arg.id(), arg.default_values().front());
            }
        }
    }

    insert_default_values(matches);

    insert_env_values(matches);

    if (!disable_help_flag_ && (matches.get_flag("help") || matches.get_flag("h"))) {
        return ParseResult("DisplayHelp");
    }

    if (!disable_version_flag_ && (matches.get_flag("version") || matches.get_flag("V"))) {
        return ParseResult("DisplayVersion");
    }

    for (const auto& arg : args_) {
        if (arg.required() && !matches.contains_id(arg.id())) {
            if (!arg.is_positional() || pos_idx > positional_args.size()) {
                std::string name = arg_display_name(arg);
                return ParseResult("error: required argument '" + name + "' not provided");
            }
        }
    }

    auto conflict_result = check_conflicts(matches);
    if (!conflict_result.success) return conflict_result;
    matches = std::move(conflict_result.matches);

    auto requires_result = check_requires(matches);
    if (!requires_result.success) return requires_result;
    matches = std::move(requires_result.matches);

    auto group_result = check_groups(matches);
    if (!group_result.success) return group_result;
    matches = std::move(group_result.matches);

    auto num_args_result = check_num_args_range(matches);
    if (!num_args_result.success) return num_args_result;
    matches = std::move(num_args_result.matches);

    return ParseResult(std::move(matches));
}

void Command::print_help() const {
    std::cout << name_;
    if (!version_.empty()) {
        std::cout << " " << version_;
    }
    std::cout << "\n";
    if (!about_.empty()) {
        std::cout << about_ << "\n";
    }
    std::cout << "\nUsage: " << name_;

    for (const auto& arg : args_) {
        if (arg.is_positional()) {
            std::cout << " <" << arg.id() << ">";
        } else {
            std::cout << " [";
            if (arg.short_opt().is_some()) {
                std::cout << "-" << arg.short_opt().unwrap();
                if (arg.long_opt().is_some()) std::cout << ", ";
            }
            if (arg.long_opt().is_some()) {
                std::cout << "--" << arg.long_opt().unwrap();
            }
            if (arg.takes_values()) {
                std::string vn = arg.value_names().empty() ? arg.id() : arg.value_names().front();
                std::cout << " <" << vn << ">";
            }
            std::cout << "]";
        }
    }
    std::cout << "\n\nOptions:\n";

    for (const auto& arg : args_) {
        if (!arg.is_positional()) {
            std::string opt_str = "  ";
            if (arg.short_opt().is_some()) {
                opt_str += "-" + std::string(1, arg.short_opt().unwrap());
                if (arg.long_opt().is_some()) opt_str += ", ";
            } else {
                opt_str += "    ";
            }
            if (arg.long_opt().is_some()) {
                opt_str += "--" + arg.long_opt().unwrap();
            }
            if (arg.takes_values()) {
                std::string vn = arg.value_names().empty() ? arg.id() : arg.value_names().front();
                opt_str += " <" + vn + ">";
            }
            while (opt_str.size() < 30) opt_str += " ";
            if (arg.help().is_some()) {
                opt_str += arg.help().unwrap();
            }
            std::cout << opt_str << "\n";
        }
    }

    if (!disable_help_flag_) {
        std::cout << "  -h, --help           Print help\n";
    }
    if (!disable_version_flag_) {
        std::cout << "  -V, --version        Print version\n";
    }
}

void Command::print_version() const {
    std::cout << name_;
    if (!version_.empty()) {
        std::cout << " " << version_;
    }
    std::cout << "\n";
}

std::string Command::arg_display_name(const Arg& arg) const {
    if (arg.long_opt().is_some()) {
        return "--" + arg.long_opt().unwrap();
    }
    if (arg.short_opt().is_some()) {
        return "-" + std::string(1, arg.short_opt().unwrap());
    }
    return arg.id();
}

std::pair<size_t, size_t> Command::get_min_max_values(const Arg& arg) const {
    size_t min_values = 1, max_values = 1;
    if (arg.num_args().is_some()) {
        const auto& range = arg.num_args().unwrap();
        if (range.min.has_value()) min_values = range.min.value();
        if (range.max.has_value()) max_values = range.max.value();
        else max_values = SIZE_MAX;
    }
    return {min_values, max_values};
}

size_t Command::count_total_values(const Arg& arg, const std::vector<std::string>& raw_values) const {
    if (!arg.value_delimiter().is_some()) {
        return raw_values.size();
    }
    char delim = arg.value_delimiter().unwrap();
    size_t total = 0;
    for (const auto& rv : raw_values) {
        size_t start = 0;
        while (start < rv.size()) {
            size_t end = rv.find(delim, start);
            if (end == std::string::npos) {
                total++;
                break;
            }
            total++;
            start = end + 1;
        }
    }
    return total;
}

void Command::insert_values_with_delimiter(const Arg& arg, ArgMatches& matches, const std::vector<std::string>& raw_values, ArgAction action, const ValueParser* parser) const {
    char delim = arg.value_delimiter().unwrap();
    if (action == ArgAction::Set) {
        matches.clear_values(arg.id());
    }
    for (const auto& val : raw_values) {
        size_t start = 0;
        while (start < val.size()) {
            size_t end = val.find(delim, start);
            std::string part;
            if (end == std::string::npos) {
                part = val.substr(start);
            } else {
                part = val.substr(start, end - start);
            }

            std::string parsed_val = part;
            if (parser && !parser->parse(part, parsed_val)) {
                parsed_val = part;
            }
            matches.insert_value(arg.id(), parsed_val);

            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }
}

std::vector<std::string> Command::collect_raw_values(const std::vector<std::string>& argv, size_t& i, size_t max_values, const std::string& initial_value) const {
    std::vector<std::string> raw_values;
    if (!initial_value.empty()) {
        raw_values.push_back(initial_value);
    }
    while (raw_values.size() < max_values && i + 1 < argv.size()) {
        const std::string& next = argv[i + 1];
        if (!next.empty() && next[0] == '-') {
            break;
        }
        raw_values.push_back(argv[++i]);
    }
    return raw_values;
}

ParseResult Command::validate_and_insert_values(const Arg& arg, ArgMatches& matches, const std::vector<std::string>& raw_values, const std::string& current) const {
    auto [min_values, max_values] = get_min_max_values(arg);
    size_t total_values = count_total_values(arg, raw_values);

    if (total_values < min_values) {
        if (get_setting(CommandSetting::IgnoreErrors)) {
            // Skip validation and return success
            return ParseResult(std::move(matches));
        }
        return ParseResult("error: argument '" + current + "' requires at least " + std::to_string(min_values) + " values");
    }

    auto action = arg.action().is_some() ? arg.action().unwrap() : ArgAction::Set;

    const ValueParser* parser = arg.value_parser();

    if (arg.value_delimiter().is_some()) {
        insert_values_with_delimiter(arg, matches, raw_values, action, parser);
    } else {
        if (action == ArgAction::Set) {
            matches.clear_values(arg.id());
        }
        for (const auto& val : raw_values) {
            std::string parsed_val = val;
            if (parser) {
                if (!parser->parse(val, parsed_val)) {
                    return ParseResult("error: invalid value '" + val + "' for argument '" + current + "', expected " + parser->type_name());
                }
            }
            matches.insert_value(arg.id(), parsed_val);
        }
    }
    return ParseResult(std::move(matches));
}

void Command::insert_default_values(ArgMatches& matches) const {
    for (const auto& arg : args_) {
        if (!arg.is_positional() && !matches.contains_id(arg.id())) {
            bool applied_conditional = false;
            for (const auto& cd : arg.default_value_ifs()) {
                bool predicate_matches = false;
                const auto& pred = cd.predicate;

                if (pred.type == ArgPredicateType::IsPresent) {
                    predicate_matches = matches.contains_id(pred.arg_id);
                    if (cd.is_unless) {
                        predicate_matches = !predicate_matches;
                    }
                } else if (pred.type == ArgPredicateType::Equals) {
                    auto val = matches.get_one(pred.arg_id);
                    bool flag_val = matches.get_flag(pred.arg_id);
                    if (val.is_some() || flag_val) {
                        std::string check_val = val.is_some() ? val.unwrap() : (flag_val ? "true" : "false");
                        if (cd.is_if_all) {
                            for (const auto& v : pred.values) {
                                if (check_val == v) {
                                    predicate_matches = true;
                                    break;
                                }
                            }
                        } else {
                            if (!pred.values.empty() && check_val == pred.values.front()) {
                                predicate_matches = true;
                            }
                        }
                    }
                } else if (pred.type == ArgPredicateType::NotEquals) {
                    auto val = matches.get_one(pred.arg_id);
                    bool flag_val = matches.get_flag(pred.arg_id);
                    if (val.is_some() || flag_val) {
                        std::string check_val = val.is_some() ? val.unwrap() : (flag_val ? "true" : "false");
                        if (cd.is_unless) {
                            if (!pred.values.empty() && check_val != pred.values.front()) {
                                predicate_matches = true;
                            }
                        } else {
                            bool matches_any = false;
                            for (const auto& v : pred.values) {
                                if (check_val == v) {
                                    matches_any = true;
                                    break;
                                }
                            }
                            predicate_matches = !matches_any;
                        }
                    }
                }

                if (predicate_matches) {
                    for (const auto& v : cd.values) {
                        matches.insert_value(arg.id(), v);
                    }
                    applied_conditional = true;
                    break;
                }
            }

            if (!applied_conditional && !arg.default_values().empty()) {
                matches.insert_value(arg.id(), arg.default_values().front());
            }
        }
    }
}

void Command::insert_env_values(ArgMatches& matches) const {
    for (const auto& arg : args_) {
        if (!arg.is_positional() && !matches.contains_id(arg.id()) && arg.env().is_some()) {
            const char* env_val = std::getenv(arg.env().unwrap().c_str());
            if (env_val) {
                matches.insert_value(arg.id(), env_val);
            } else if (arg.env_default().is_some()) {
                matches.insert_value(arg.id(), arg.env_default().unwrap());
            }
        }
    }
}

ParseResult Command::check_conflicts(const ArgMatches& matches) const {
    for (const auto& arg : args_) {
        if (matches.contains_id(arg.id())) {
            for (const auto& conflict_id : arg.conflicts()) {
                if (matches.contains_id(conflict_id)) {
                    std::string name1 = arg_display_name(arg);
                    std::string name2 = conflict_id;
                    for (const auto& other_arg : args_) {
                        if (other_arg.id() == conflict_id) {
                            name2 = arg_display_name(other_arg);
                            break;
                        }
                    }
                    return ParseResult("error: argument '" + name1 + "' conflicts with '" + name2 + "'");
                }
            }
        }
    }
    return ParseResult(std::move(const_cast<ArgMatches&>(matches)));
}

ParseResult Command::check_requires(const ArgMatches& matches) const {
    for (const auto& arg : args_) {
        if (matches.contains_id(arg.id())) {
            for (const auto& req_id : arg.requires_args()) {
                if (!matches.contains_id(req_id)) {
                    std::string name1 = arg_display_name(arg);
                    std::string name2 = req_id;
                    for (const auto& other_arg : args_) {
                        if (other_arg.id() == req_id) {
                            name2 = arg_display_name(other_arg);
                            break;
                        }
                    }
                    return ParseResult("error: argument '" + name1 + "' requires '" + name2 + "'");
                }
            }
        }
    }
    return ParseResult(std::move(const_cast<ArgMatches&>(matches)));
}

ParseResult Command::check_num_args_range(const ArgMatches& matches) const {
    for (const auto& arg : args_) {
        if (arg.num_args().is_some() && matches.contains_id(arg.id())) {
            size_t count = matches.count(arg.id());
            const auto& range = arg.num_args().unwrap();
            if (range.min.has_value() && count < range.min.value()) {
                return ParseResult("error: argument '" + arg.id() + "' requires at least " + std::to_string(range.min.value()) + " values");
            }
            if (range.max.has_value() && count > range.max.value()) {
                return ParseResult("error: argument '" + arg.id() + "' requires at most " + std::to_string(range.max.value()) + " values");
            }
        }
    }
    return ParseResult(std::move(const_cast<ArgMatches&>(matches)));
}

ParseResult Command::check_groups(const ArgMatches& matches) const {
    ArgMatches new_matches = matches;
    for (const auto& group : groups_) {
        size_t group_count = 0;
        std::string used_arg;
        for (const auto& arg_id : group.args()) {
            if (matches.contains_id(arg_id)) {
                group_count++;
                used_arg = arg_id;
            }
        }

        if (group.required() && group_count == 0) {
            return ParseResult("error: required argument group '" + group.id() + "' not provided");
        }

        if (!group.multiple() && group_count > 1) {
            return ParseResult("error: argument group '" + group.id() + "' allows only one argument to be used");
        }

        if (group_count > 0) {
            for (const auto& req_id : group.requires_args()) {
                if (!matches.contains_id(req_id)) {
                    std::string name1 = group.id();
                    std::string name2 = req_id;
                    for (const auto& other_arg : args_) {
                        if (other_arg.id() == req_id) {
                            name2 = arg_display_name(other_arg);
                            break;
                        }
                    }
                    for (const auto& other_group : groups_) {
                        if (other_group.id() == req_id) {
                            name2 = other_group.id();
                            break;
                        }
                    }
                    return ParseResult("error: argument group '" + name1 + "' requires '" + name2 + "'");
                }
            }

            for (const auto& conflict_id : group.conflicts()) {
                if (matches.contains_id(conflict_id)) {
                    std::string name1 = group.id();
                    std::string name2 = conflict_id;
                    for (const auto& other_arg : args_) {
                        if (other_arg.id() == conflict_id) {
                            name2 = arg_display_name(other_arg);
                            break;
                        }
                    }
                    for (const auto& other_group : groups_) {
                        if (other_group.id() == conflict_id) {
                            name2 = other_group.id();
                            break;
                        }
                    }
                    return ParseResult("error: argument group '" + name1 + "' conflicts with '" + name2 + "'");
                }
            }

            if (!used_arg.empty()) {
                new_matches.insert_value(group.id(), used_arg);
            }
        }
    }
    return ParseResult(std::move(new_matches));
}

std::vector<std::string> Command::collect_raw_values(const std::vector<std::string>& argv, size_t& i, size_t max_values, const std::string& initial_value, const Arg& arg) const {
    std::vector<std::string> raw_values;
    if (!initial_value.empty()) {
        raw_values.push_back(initial_value);
    }
    while (raw_values.size() < max_values && i + 1 < argv.size()) {
        const std::string& next = argv[i + 1];
        bool is_negative = is_negative_number(next);
        bool is_option = !next.empty() && next[0] == '-';

        // Check for value terminator
        if (arg.value_terminator().is_some() && next == arg.value_terminator().unwrap()) {
            // Consume the terminator but don't add it to values
            ++i;
            break;
        }

        if (is_option && !is_negative) {
            if (arg.allow_negative_numbers() && is_negative) {
                raw_values.push_back(argv[++i]);
            } else if (arg.allow_hyphen_values()) {
                raw_values.push_back(argv[++i]);
            } else {
                break;
            }
        } else if (is_option && is_negative) {
            if (arg.allow_negative_numbers()) {
                raw_values.push_back(argv[++i]);
            } else if (arg.allow_hyphen_values()) {
                raw_values.push_back(argv[++i]);
            } else {
                break;
            }
        } else {
            raw_values.push_back(argv[++i]);
        }
    }

    return raw_values;
}

bool Command::is_negative_number(const std::string& s) {
    if (s.empty() || s[0] != '-') return false;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

std::string Command::render_help() const {
    std::ostringstream oss;
    oss << name_;
    if (!version_.empty()) {
        oss << " " << version_;
    }
    oss << "\n";
    if (!about_.empty()) {
        oss << about_ << "\n";
    }
    oss << "\nUsage: " << name_;

    // Sort non-positional args by display_order
    std::vector<const Arg*> sorted_args;
    for (const auto& arg : args_) {
        if (!arg.is_positional()) {
            sorted_args.push_back(&arg);
        }
    }
    std::sort(sorted_args.begin(), sorted_args.end(), [](const Arg* a, const Arg* b) {
        auto a_order = a->display_order().is_some() ? a->display_order().unwrap() : SIZE_MAX;
        auto b_order = b->display_order().is_some() ? b->display_order().unwrap() : SIZE_MAX;
        return a_order < b_order;
    });

    // Use sorted_args for usage line as well
    for (const auto* arg : sorted_args) {
        if (arg->is_positional()) {
            oss << " <" << arg->id() << ">";
        } else {
            oss << " [";
            if (arg->short_opt().is_some()) {
                oss << "-" << arg->short_opt().unwrap();
                if (arg->long_opt().is_some()) oss << ", ";
            }
            if (arg->long_opt().is_some()) {
                oss << "--" << arg->long_opt().unwrap();
            }
            if (arg->takes_values()) {
                std::string vn = arg->value_names().empty() ? arg->id() : arg->value_names().front();
                oss << " <" << vn << ">";
            }
            oss << "]";
        }
    }
    oss << "\n\nOptions:\n";

    for (const auto* arg : sorted_args) {
        if (!arg->is_positional()) {
            std::string opt_str = "  ";
            if (arg->short_opt().is_some()) {
                opt_str += "-" + std::string(1, arg->short_opt().unwrap());
                if (arg->long_opt().is_some()) opt_str += ", ";
            } else {
                opt_str += "    ";
            }
            if (arg->long_opt().is_some()) {
                opt_str += "--" + arg->long_opt().unwrap();
            }
            if (arg->takes_values()) {
                std::string vn = arg->value_names().empty() ? arg->id() : arg->value_names().front();
                opt_str += " <" + vn + ">";
            }
            while (opt_str.size() < 30) opt_str += " ";
            if (arg->help().is_some()) {
                opt_str += arg->help().unwrap();
            }
            oss << opt_str << "\n";
        }
    }

    if (!disable_help_flag_) {
        oss << "  -h, --help           Print help\n";
    }
    if (!disable_version_flag_) {
        oss << "  -V, --version        Print version\n";
    }

    return oss.str();
}

std::string Command::render_long_help() const {
    return render_help();
}

std::string Command::render_version() const {
    std::ostringstream oss;
    oss << name_;
    if (!version_.empty()) {
        oss << " " << version_;
    }
    oss << "\n";
    return oss.str();
}

std::string Command::render_long_version() const {
    return render_version();
}

std::string Command::render_usage() const {
    std::ostringstream oss;
    oss << "Usage: " << name_;

    for (const auto& arg : args_) {
        if (arg.is_positional()) {
            oss << " <" << arg.id() << ">";
        } else {
            oss << " [";
            if (arg.short_opt().is_some()) {
                oss << "-" << arg.short_opt().unwrap();
                if (arg.long_opt().is_some()) oss << ", ";
            }
            if (arg.long_opt().is_some()) {
                oss << "--" << arg.long_opt().unwrap();
            }
            if (arg.takes_values()) {
                std::string vn = arg.value_names().empty() ? arg.id() : arg.value_names().front();
                oss << " <" << vn << ">";
            }
            oss << "]";
        }
    }
    oss << "\n";
    return oss.str();
}

} // namespace dwhbll::cli
