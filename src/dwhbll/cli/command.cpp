#include <dwhbll/cli/command.h>
#include <iostream>
#include <cstdlib>

extern int __argc;
extern char** __argv;

namespace dwhbll::cli {

ParseResult Command::try_get_matches_from(const std::vector<std::string>& argv) const {
    return parse_args(argv);
}

ArgMatches Command::get_matches_from(const std::vector<std::string>& argv) const {
    auto result = try_get_matches_from(argv);
    if (!result.success) {
        for (const auto& err : result.errors) {
            std::cerr << err << "\n";
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
                    return ParseResult("error: unknown argument '" + current + "'");
                }

                const Arg* arg = it->second;
                if (arg->action().is_some()) {
                    auto action = arg->action().unwrap();
                    if (action == ArgAction::SetTrue || action == ArgAction::SetFalse ||
                        action == ArgAction::Count || action == ArgAction::Help ||
                        action == ArgAction::Version) {
                        if (!value.empty()) {
                            return ParseResult("error: flag '" + current + "' doesn't take a value");
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
                        auto raw_values = collect_raw_values(argv, i, max_values, value);
                        auto result = validate_and_insert_values(*arg, matches, raw_values, current);
                        if (!result.success) return result;
                        matches = std::move(result.matches);
                    }
                } else {
                    auto action = arg->action().is_some() ? arg->action().unwrap() : ArgAction::Set;
                    auto [min_values, max_values] = get_min_max_values(*arg);
                    std::string value;
                    if (i + 1 >= argv.size()) {
                        return ParseResult("error: argument '" + current + "' requires a value");
                    }
                    value = argv[++i];
                    auto raw_values = collect_raw_values(argv, i, max_values, value);
                    auto result = validate_and_insert_values(*arg, matches, raw_values, current);
                    if (!result.success) return result;
                    matches = std::move(result.matches);
                }
            } else {
                std::string short_flags = current.substr(1);
                for (size_t j = 0; j < short_flags.size(); ++j) {
                    char flag = short_flags[j];
                    auto it = short_map.find(flag);
                    if (it == short_map.end()) {
                        return ParseResult("error: unknown argument '-" + std::string(1, flag) + "'");
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
                                return ParseResult("error: argument '-" + std::string(1, flag) + "' requires a value");
                            }

                            auto raw_values = collect_raw_values(argv, i, max_values, value);
                            auto result = validate_and_insert_values(*arg, matches, raw_values, "-" + std::string(1, flag));
                            if (!result.success) return result;
                            matches = std::move(result.matches);
                        }
                    } else {
                        std::string value;
                        if (j + 1 < short_flags.size()) {
                            value = short_flags.substr(j + 1);
                            j = short_flags.size();
                        } else if (i + 1 < argv.size()) {
                            value = argv[++i];
                        } else {
                            return ParseResult("error: argument '-" + std::string(1, flag) + "' requires a value");
                        }
                        matches.insert_value(arg->id(), value);
                    }
                }
            }
        } else {
            if (!subcommands_.empty()) {
                auto sub_it = subcommands_.find(current);
                if (sub_it != subcommands_.end()) {
                    std::vector<std::string> sub_argv(argv.begin() + i + 1, argv.end());
                    auto sub_result = sub_it->second.try_get_matches_from(sub_argv);
                    if (!sub_result.success) {
                        return sub_result;
                    }
                    matches.insert_subcommand(current, std::make_unique<ArgMatches>(std::move(sub_result.matches)));
                    positional_args.push_back(current);
                    break;
                }
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
        print_help();
        std::exit(0);
    }

    if (!disable_version_flag_ && (matches.get_flag("version") || matches.get_flag("V"))) {
        print_version();
        std::exit(0);
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

void Command::insert_values_with_delimiter(const Arg& arg, ArgMatches& matches, const std::vector<std::string>& raw_values, ArgAction action) const {
    char delim = arg.value_delimiter().unwrap();
    if (action == ArgAction::Set) {
        matches.clear_values(arg.id());
    }
    for (const auto& val : raw_values) {
        size_t start = 0;
        while (start < val.size()) {
            size_t end = val.find(delim, start);
            if (end == std::string::npos) {
                matches.insert_value(arg.id(), val.substr(start));
                break;
            }
            matches.insert_value(arg.id(), val.substr(start, end - start));
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
        return ParseResult("error: argument '" + current + "' requires at least " + std::to_string(min_values) + " values");
    }

    auto action = arg.action().is_some() ? arg.action().unwrap() : ArgAction::Set;

    if (arg.value_delimiter().is_some()) {
        insert_values_with_delimiter(arg, matches, raw_values, action);
    } else {
        if (action == ArgAction::Set) {
            matches.clear_values(arg.id());
        }
        for (const auto& val : raw_values) {
            matches.insert_value(arg.id(), val);
        }
    }
    return ParseResult(std::move(matches));
}

void Command::insert_default_values(ArgMatches& matches) const {
    for (const auto& arg : args_) {
        if (!arg.is_positional() && !matches.contains_id(arg.id()) && !arg.default_values().empty()) {
            matches.insert_value(arg.id(), arg.default_values().front());
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

} // namespace dwhbll::cli
