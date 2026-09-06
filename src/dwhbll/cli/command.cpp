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

    // Add help and version flags if not disabled
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
                        if (value.empty()) {
                            if (i + 1 >= argv.size()) {
                                return ParseResult("error: argument '" + current + "' requires a value");
                            }
                            value = argv[++i];
                        }
                        matches.insert_value(arg->id(), value);
                    }
                } else {
                    if (value.empty()) {
                        if (i + 1 >= argv.size()) {
                            return ParseResult("error: argument '" + current + "' requires a value");
                        }
                        value = argv[++i];
                    }
                    matches.insert_value(arg->id(), value);
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
            if (pos_idx <= positional_args.size()) {
                matches.insert_value(arg.id(), positional_args[pos_idx - 1]);
            } else if (arg.required()) {
                return ParseResult("error: required positional argument '" + arg.id() + "' not provided");
            } else if (!arg.default_values().empty()) {
                matches.insert_value(arg.id(), arg.default_values().front());
            }
            ++pos_idx;
        }
    }

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
                std::string name = arg.long_opt().is_some() ? "--" + arg.long_opt().unwrap() :
                                (arg.short_opt().is_some() ? "-" + std::string(1, arg.short_opt().unwrap()) : arg.id());
                return ParseResult("error: required argument '" + name + "' not provided");
            }
        }
    }

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

} // namespace dwhbll::cli
