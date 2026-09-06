#include <dwhbll/cli/command.h>
#include <iostream>
#include <vector>

bool cli_test(std::optional<std::string> test_to_run) {
    using namespace dwhbll::cli;

    bool all_passed = true;

    // option parsing
    {
        Command cmd("test1");
        cmd.arg(Arg("config")
            .short_opt('c')
            .long_opt("config")
            .help("Configuration file")
            .action(ArgAction::Set)
            .value_name("FILE")
            .required(true));

        std::vector<std::string> args = {"-c", "config.txt"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("config").unwrap() != "config.txt") {
            std::cerr << "Test 1 failed: basic option parsing\n";
            all_passed = false;
        }
    }

    // flag parsing
    {
        Command cmd("test2");
        cmd.arg(Arg("verbose")
            .short_opt('v')
            .long_opt("verbose")
            .help("Enable verbose output")
            .action(ArgAction::SetTrue));

        std::vector<std::string> args = {"-v"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || !result.matches.get_flag("verbose")) {
            std::cerr << "Test 2 failed: flag parsing\n";
            all_passed = false;
        }
    }

    // default values
    {
        Command cmd("test3");
        cmd.arg(Arg("count")
            .short_opt('n')
            .long_opt("count")
            .help("Number of iterations")
            .action(ArgAction::Set)
            .value_name("NUM")
            .default_value("10"));

        std::vector<std::string> args = {};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("count").unwrap() != "10") {
            std::cerr << "Test 3 failed: default values\n";
            all_passed = false;
        }
    }

    // positional arguments
    {
        Command cmd("test4");
        cmd.arg(Arg("input").help("Input file"));
        cmd.arg(Arg("output").help("Output file"));

        std::vector<std::string> args = {"input.txt", "output.txt"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("input").unwrap() != "input.txt" ||
            result.matches.get_one("output").unwrap() != "output.txt") {
            std::cerr << "Test 4 failed: positional arguments\n";
            all_passed = false;
        }
    }

    // long option with equals
    {
        Command cmd("test5");
        cmd.arg(Arg("config")
            .long_opt("config")
            .help("Configuration file")
            .action(ArgAction::Set)
            .value_name("FILE"));

        std::vector<std::string> args = {"--config=config.txt"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("config").unwrap() != "config.txt") {
            std::cerr << "Test 5 failed: long option with equals\n";
            all_passed = false;
        }
    }

    // short options combined
    {
        Command cmd("test6");
        cmd.arg(Arg("a").short_opt('a').action(ArgAction::SetTrue));
        cmd.arg(Arg("b").short_opt('b').action(ArgAction::SetTrue));
        cmd.arg(Arg("c").short_opt('c').action(ArgAction::SetTrue));

        std::vector<std::string> args = {"-abc"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || !result.matches.get_flag("a") || !result.matches.get_flag("b") || !result.matches.get_flag("c")) {
            std::cerr << "Test 6 failed: short options combined\n";
            all_passed = false;
        }
    }

    // value delimiter
    {
        Command cmd("test7");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("items")
            .long_opt("items")
            .help("List of items")
            .action(ArgAction::Set)
            .value_delimiter(',')
            .value_name("ITEM"));

        std::vector<std::string> args = {"--items=a,b,c"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_many("items").size() != 3) {
            std::cerr << "Test 7 failed: value delimiter\n";
            all_passed = false;
        }
    }

    // count action
    {
        Command cmd("test8");
        cmd.arg(Arg("verbose")
            .short_opt('v')
            .long_opt("verbose")
            .help("Verbosity level")
            .action(ArgAction::Count));

        std::vector<std::string> args = {"-vvv"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.count("verbose") != 3) {
            std::cerr << "Test 8 failed: count action\n";
            all_passed = false;
        }
    }

    // append action
    {
        Command cmd("test8b");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("items")
            .long_opt("items")
            .action(ArgAction::Append)
            .value_name("ITEM"));

        std::vector<std::string> args = {"--items=a", "--items=b", "--items=c"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_many("items").size() != 3) {
            std::cerr << "Test 8b failed: append action\n";
            all_passed = false;
        } else {
            auto values = result.matches.get_many("items");
            if (values[0] != "a" || values[1] != "b" || values[2] != "c") {
                std::cerr << "Test 8b failed: append action values incorrect\n";
                all_passed = false;
            }
        }
    }

    // SetFalse action
    {
        Command cmd("test8c");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("no_color")
            .long_opt("no-color")
            .action(ArgAction::SetFalse)
            .help("Disable color output"));

        std::vector<std::string> args = {"--no-color"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_flag("no_color") != false) {
            std::cerr << "Test 8c failed: setfalse action\n";
            all_passed = false;
        }
    }

    // trailing_var_arg
    {
        Command cmd("test8d");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("cmd").help("Command to run"));
        cmd.arg(Arg("args").help("Arguments").trailing_var_arg(true));

        std::vector<std::string> args = {"mycmd", "arg1", "arg2", "arg3"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("cmd").unwrap() != "mycmd" ||
            result.matches.get_many("args").size() != 3) {
            std::cerr << "Test 8d failed: trailing_var_arg\n";
            all_passed = false;
        }
    }

    // num_args validation
    {
        Command cmd("test8e");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("files")
            .long_opt("files")
            .action(ArgAction::Set)
            .num_args(ValueRange::at_least(2))
            .value_name("FILE"));

        std::vector<std::string> args = {"--files", "a.txt"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 8e failed: num_args validation (too few)\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--files", "a.txt", "b.txt"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_many("files").size() != 2) {
            std::cerr << "Test 8e failed: num_args validation (correct count)\n";
            all_passed = false;
        }
    }

    // num_args with value delimiter
    {
        Command cmd("test8f");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("items")
            .long_opt("items")
            .action(ArgAction::Set)
            .value_delimiter(',')
            .num_args(ValueRange::fixed(3))
            .value_name("ITEM"));

        std::vector<std::string> args = {"--items=a,b"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 8f failed: num_args with delimiter (too few)\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--items=a,b,c"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_many("items").size() != 3) {
            std::cerr << "Test 8f failed: num_args with delimiter (correct count)\n";
            all_passed = false;
        }
    }

    // subcommands
    {
        Command cmd("test9");
        cmd.subcommand(Command("sub1")
            .arg(Arg("arg1").help("Sub arg 1")));

        std::vector<std::string> args = {"sub1", "value1"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 9 failed: subcommands basic\n";
            all_passed = false;
        } else {
            auto sub_name = result.matches.subcommand_name();
            if (!sub_name.is_some() || sub_name.unwrap() != "sub1") {
                std::cerr << "Test 9 failed: subcommand name not found\n";
                all_passed = false;
            }
            const ArgMatches* sub_matches = result.matches.subcommand_matches("sub1");
            if (!sub_matches || sub_matches->get_one("arg1").unwrap() != "value1") {
                std::cerr << "Test 9 failed: subcommand matches not correct\n";
                all_passed = false;
            }
        }
    }

    // Subcommand with options
    {
        Command build_sub("build");
        build_sub.disable_help_flag(true).disable_version_flag(true);
        build_sub.arg(Arg("target").short_opt('t').long_opt("target").action(ArgAction::Set).value_name("TARGET"));
        build_sub.arg(Arg("release").short_opt('r').long_opt("release").action(ArgAction::SetTrue));

        Command cmd("test9b");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.subcommand(std::move(build_sub));

        std::vector<std::string> args = {"build", "--target", "x86_64", "--release"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 9b failed: subcommand with options - " << result.errors[0] << "\n";
            all_passed = false;
        } else {
            const ArgMatches* sub_matches = result.matches.subcommand_matches("build");
            if (!sub_matches || sub_matches->get_one("target").unwrap() != "x86_64" || !sub_matches->get_flag("release")) {
                std::cerr << "Test 9b failed: subcommand with options not correct\n";
                all_passed = false;
            }
        }
    }

    // required argument missing
    {
        Command cmd("test10");
        cmd.arg(Arg("required_arg")
            .long_opt("required")
            .help("Required argument")
            .action(ArgAction::Set)
            .required(true));

        std::vector<std::string> args = {};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 10 failed: required argument missing should fail\n";
            all_passed = false;
        }
    }

    // literals
    {
        using namespace dwhbll::cli::literals;
        Arg a = "test_literal2"_Arg;
        if (a.id() != "test_literal2") {
            std::cerr << "Test 11 failed: make_arg and literals\n";
            all_passed = false;
        }
    }

    // subcommand help
    {
        Command cmd("test12");
        cmd.disable_help_flag(false).disable_version_flag(true);
        cmd.subcommand(Command("build")
            .disable_help_flag(false)
            .disable_version_flag(true)
            .arg(Arg("target").short_opt('t').long_opt("target").action(ArgAction::Set).value_name("TARGET")));

        std::vector<std::string> args = {"build", "--help"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 12 failed: subcommand help should exit\n";
            all_passed = false;
        }
    }

    // conflicts validation
    {
        Command cmd("test13");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue).conflicts_with("opt2"));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));

        std::vector<std::string> args = {"--opt1", "--opt2"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 13 failed: conflicts validation\n";
            all_passed = false;
        } else if (result.errors[0].find("conflicts") == std::string::npos) {
            std::cerr << "Test 13 failed: conflicts error message: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // requires validation
    {
        Command cmd("test14");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::Set).requires_arg("opt2"));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::Set));

        std::vector<std::string> args = {"--opt1", "value1"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 14 failed: requires validation\n";
            all_passed = false;
        } else if (result.errors[0].find("requires") == std::string::npos) {
            std::cerr << "Test 14 failed: requires error message: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // requires with both present
    {
        Command cmd("test15");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::Set).requires_arg("opt2"));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::Set));

        std::vector<std::string> args = {"--opt1", "value1", "--opt2", "value2"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 15 failed: requires with both present: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // conflicts with both not present
    {
        Command cmd("test16");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue).conflicts_with("opt2"));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));

        std::vector<std::string> args = {};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 16 failed: conflicts with neither present: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // environment variable support
    {
        setenv("TEST_CLI_ENV_VAR", "env_value", 1);

        Command cmd("test17");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("from_env").long_opt("from-env").action(ArgAction::Set).env("TEST_CLI_ENV_VAR"));

        std::vector<std::string> args = {};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("from_env").unwrap() != "env_value") {
            std::cerr << "Test 17 failed: environment variable not used\n";
            all_passed = false;
        }

        unsetenv("TEST_CLI_ENV_VAR");
        Command cmd2("test17b");
        cmd2.disable_help_flag(true).disable_version_flag(true);
        cmd2.arg(Arg("from_env_default").long_opt("from-env-default").action(ArgAction::Set)
            .env("NONEXISTENT_VAR", dwhbll::stl_ext::Option<std::string>(std::string("default_value"))));

        std::vector<std::string> args2 = {};
        auto result2 = cmd2.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_one("from_env_default").unwrap() != "default_value") {
            std::cerr << "Test 17b failed: environment variable default not used\n";
            all_passed = false;
        }

        unsetenv("TEST_CLI_ENV_VAR");
    }

    if (all_passed) {
        std::cout << "All CLI tests passed!\n";
    }
    return all_passed;
}
