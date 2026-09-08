#include <dwhbll/cli/command.h>
#include <iostream>
#include <vector>

bool cli_test(std::optional<std::string>) {
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

    // ArgGroup
    {
        Command cmd("test18");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt3").long_opt("opt3").action(ArgAction::SetTrue));
        cmd.group(ArgGroup("group1")
            .args({"opt1", "opt2"})
            .required(true));

        std::vector<std::string> args = {"--opt1"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 18 failed: ArgGroup required with one arg: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // ArgGroup multiple
    {
        Command cmd("test19");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
        cmd.group(ArgGroup("group1")
            .args({"opt1", "opt2"})
            .multiple(true));

        std::vector<std::string> args = {"--opt1", "--opt2"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 19 failed: ArgGroup multiple: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // ArgGroup conflict
    {
        Command cmd("test20");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt3").long_opt("opt3").action(ArgAction::SetTrue));
        cmd.group(ArgGroup("group1")
            .args({"opt1", "opt2"})
            .conflicts_with("opt3"));

        std::vector<std::string> args = {"--opt1", "--opt3"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 20 failed: ArgGroup conflict should fail\n";
            all_passed = false;
        }
    }

    // allow_hyphen_values
    {
        Command cmd("test21");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("pattern")
            .long_opt("pattern")
            .action(ArgAction::Set)
            .allow_hyphen_values(true)
            .value_name("PATTERN"));

        std::vector<std::string> args = {"--pattern", "-file.txt"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("pattern").unwrap() != "-file.txt") {
            std::cerr << "Test 21 failed: allow_hyphen_values\n";
            all_passed = false;
        }
    }

    // allow_negative_numbers
    {
        Command cmd("test22");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("num")
            .long_opt("num")
            .action(ArgAction::Set)
            .allow_negative_numbers(true)
            .value_name("NUM"));

        std::vector<std::string> args = {"--num", "-42"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("num").unwrap() != "-42") {
            std::cerr << "Test 22 failed: allow_negative_numbers\n";
            all_passed = false;
        }
    }

    // require_equals
    {
        Command cmd("test23");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("config")
            .long_opt("config")
            .action(ArgAction::Set)
            .require_equals(true)
            .value_name("FILE"));

        std::vector<std::string> args = {"--config=config.txt"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("config").unwrap() != "config.txt") {
            std::cerr << "Test 23 failed: require_equals with equals\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--config", "config.txt"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (result2.success) {
            std::cerr << "Test 23b failed: require_equals without equals should fail\n";
            all_passed = false;
        }
    }

    // value_terminator
    {
        Command cmd("test24");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("cmds")
            .long_opt("cmds")
            .action(ArgAction::Set)
            .num_args(ValueRange::at_least(1))
            .value_terminator(";")
            .allow_hyphen_values(true)
            .value_name("CMD"));
        cmd.arg(Arg("location")
            .long_opt("location")
            .action(ArgAction::Set)
            .value_name("LOC"));

        std::vector<std::string> args = {"--cmds", "find", "--type", "f", ";", "--location", "/home"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 24 failed: value_terminator: " << result.errors[0] << "\n";
            all_passed = false;
        } else {
            auto cmds = result.matches.get_many("cmds");
            if (cmds.size() != 3 || cmds[0] != "find" || cmds[1] != "--type" || cmds[2] != "f") {
                std::cerr << "Test 24 failed: value_terminator values incorrect\n";
                all_passed = false;
            }
            if (result.matches.get_one("location").unwrap() != "/home") {
                std::cerr << "Test 24 failed: location not parsed\n";
                all_passed = false;
            }
        }
    }

    // raw
    {
        Command cmd("test25");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("args")
            .long_opt("args")
            .raw(true)
            .value_name("ARGS"));

        std::vector<std::string> args = {"--args", "-v", "-v", "--flag", "value"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 25 failed: raw: " << result.errors[0] << "\n";
            all_passed = false;
        } else {
            auto args_vec = result.matches.get_many("args");
            if (args_vec.size() != 4) {
                std::cerr << "Test 25 failed: raw values count incorrect\n";
                all_passed = false;
            }
        }
    }

    // ignore_case
    {
        Command cmd("test26");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("mode")
            .long_opt("mode")
            .action(ArgAction::Set)
            .value_parser({"FAST", "SLOW"}));

        std::vector<std::string> args = {"--mode", "FAST"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("mode").unwrap() != "FAST") {
            std::cerr << "Test 26 failed: value_parser exact match\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--mode", "SLOW"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_one("mode").unwrap() != "SLOW") {
            std::cerr << "Test 26b failed: value_parser exact match\n";
            all_passed = false;
        }
    }

    // default_value_if
    {
        Command cmd("test27");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("flag").long_opt("flag").action(ArgAction::SetTrue));
        cmd.arg(Arg("value")
            .long_opt("value")
            .action(ArgAction::Set)
            .default_value_if(ArgPredicate::equals("flag", "true"), "conditional_default"));

        std::vector<std::string> args = {"--flag", "--value", "explicit"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("value").unwrap() != "explicit") {
            std::cerr << "Test 27 failed: default_value_if with explicit value\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--flag"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_one("value").unwrap() != "conditional_default") {
            std::cerr << "Test 27b failed: default_value_if triggered: " << result2.errors[0] << "\n";
            all_passed = false;
        }
    }

    // default_value_unless
    {
        Command cmd("test28");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("debug").long_opt("debug").action(ArgAction::SetTrue));
        cmd.arg(Arg("log_level")
            .long_opt("log-level")
            .action(ArgAction::Set)
            .default_value_unless(ArgPredicate::is_present("debug"), "debug"));

        std::vector<std::string> args = {"--debug"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 28 failed: default_value_unless with debug: " << result.errors[0] << "\n";
            all_passed = false;
        } else if (result.matches.contains_id("log_level")) {
            std::cerr << "Test 28 failed: default_value_unless should not apply when debug is present\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {};
        auto result2 = cmd.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_one("log_level").unwrap() != "debug") {
            std::cerr << "Test 28b failed: default_value_unless without debug\n";
            all_passed = false;
        }
    }

    // ValueParser
    {
        Command cmd("test29");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("port")
            .long_opt("port")
            .action(ArgAction::Set)
            .value_parser(value_parser_uint())
            .value_name("PORT"));

        std::vector<std::string> args = {"--port", "8080"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("port").unwrap() != "8080") {
            std::cerr << "Test 29 failed: ValueParser uint: " << result.errors[0] << "\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--port", "invalid"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (result2.success) {
            std::cerr << "Test 29b failed: ValueParser should reject invalid\n";
            all_passed = false;
        }
    }

    // ValueParser bool
    {
        Command cmd("test30");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("verbose")
            .long_opt("verbose")
            .action(ArgAction::Set)
            .value_parser(value_parser_bool()));

        std::vector<std::string> args = {"--verbose", "true"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("verbose").unwrap() != "true") {
            std::cerr << "Test 30 failed: ValueParser bool true\n";
            all_passed = false;
        }

        std::vector<std::string> args2 = {"--verbose", "false"};
        auto result2 = cmd.try_get_matches_from(args2);
        if (!result2.success || result2.matches.get_one("verbose").unwrap() != "false") {
            std::cerr << "Test 30b failed: ValueParser bool false\n";
            all_passed = false;
        }
    }

    // subcommand alias
    {
        Command build_sub("build");
        build_sub.disable_help_flag(true).disable_version_flag(true);
        build_sub.alias("b");

        Command cmd("test31");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.subcommand(std::move(build_sub));

        std::vector<std::string> args = {"b"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 31 failed: subcommand alias: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // command settings
    {
        Command cmd("test32");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.no_binary_name(true);
        cmd.arg(Arg("cmd").help("Command"));
        cmd.arg(Arg("args").help("Args").trailing_var_arg(true));

        std::vector<std::string> args = {"mycmd", "arg1", "arg2"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("cmd").unwrap() != "mycmd") {
            std::cerr << "Test 32 failed: no_binary_name\n";
            all_passed = false;
        }
    }

    // external subcommand
    {
        Command cmd("test33");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.external_subcommand(true);

        std::vector<std::string> args = {"unknown_cmd", "arg1", "arg2"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 33 failed: external_subcommand: " << result.errors[0] << "\n";
            all_passed = false;
        } else if (result.matches.get_one("external_subcommand").unwrap() != "unknown_cmd") {
            std::cerr << "Test 33 failed: external_subcommand not captured\n";
            all_passed = false;
        }
    }

    // mut_arg
    {
        Command cmd("test34");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("verbose").short_opt('v').action(ArgAction::SetTrue));
        cmd.mut_arg("verbose", [](Arg a) { return a.short_opt('V'); });

        std::vector<std::string> args = {"-V"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || !result.matches.get_flag("verbose")) {
            std::cerr << "Test 34 failed: mut_arg\n";
            all_passed = false;
        }
    }

    // mut_args
    {
        Command cmd("test35");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").short_opt('o').action(ArgAction::SetTrue));
        cmd.arg(Arg("opt2").short_opt('p').action(ArgAction::SetTrue));
        cmd.mut_args([](Arg a) {
            if (a.id() == "opt1") return a.long_opt("option1");
            if (a.id() == "opt2") return a.long_opt("option2");
            return a;
        });

        std::vector<std::string> args = {"--option1", "--option2"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 35 failed: mut_args: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // mut_group
    {
        Command cmd("test36");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
        cmd.group(ArgGroup("mygroup").args({"opt1", "opt2"}));
        cmd.mut_group("mygroup", [](ArgGroup g) { return g.required(true); });

        std::vector<std::string> args = {};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 36 failed: mut_group should make required\n";
            all_passed = false;
        }
    }

    // mut_subcommand
    {
        Command build_sub("build");
        build_sub.disable_help_flag(true).disable_version_flag(true);
        build_sub.arg(Arg("target").long_opt("target").action(ArgAction::Set));

        Command cmd("test37");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.subcommand(std::move(build_sub));
        cmd.mut_subcommand("build", [](Command c) {
            return c.arg(Arg("release").long_opt("release").action(ArgAction::SetTrue));
        });

        std::vector<std::string> args = {"build", "--release"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 37 failed: mut_subcommand: " << result.errors[0] << "\n";
            all_passed = false;
        }
    }

    // mut_subcommands
    {
        Command cmd("test38");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.subcommand(Command("sub1"));
        cmd.subcommand(Command("sub2"));
        cmd.mut_subcommands([](Command c) {
            return c.disable_help_flag(true);
        });

        std::vector<std::string> args = {"sub1", "--help"};
        auto result = cmd.try_get_matches_from(args);
        if (result.success) {
            std::cerr << "Test 38 failed: mut_subcommands should disable help\n";
            all_passed = false;
        }
    }

    // Arg display_order
    {
        Command cmd("test39");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue).display_order(1));
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue).display_order(0));

        std::string help = cmd.render_help();
        size_t pos1 = help.find("--opt1");
        size_t pos2 = help.find("--opt2");
        if (pos1 > pos2) {
            std::cerr << "Test 39 failed: display_order not respected\n";
            all_passed = false;
        }
    }

    // Arg next_line_help
    {
        Command cmd("test40");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt")
            .long_opt("opt")
            .action(ArgAction::Set)
            .value_name("VALUE")
            .next_line_help(true)
            .help("This is a very long help message that should appear on the next line"));

        std::string help = cmd.render_help();
        if (help.find("This is a very long") == std::string::npos) {
            std::cerr << "Test 40 failed: next_line_help\n";
            all_passed = false;
        }
    }

    // Arg hide_short_help
    {
        Command cmd("test41");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
        cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue).hide_short_help(true));

        std::string help = cmd.render_help();
        if (help.find("--opt2") == std::string::npos) {

            // TODO: (xdd)
        }
    }

    // Arg hide_possible_values
    {
        Command cmd("test42");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("mode")
            .long_opt("mode")
            .action(ArgAction::Set)
            .value_parser({"fast", "slow"})
            .hide_possible_values(true));

        std::vector<std::string> args = {"--mode", "fast"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 42 failed: hide_possible_values\n";
            all_passed = false;
        }
    }

    // render_help / render_long_help
    {
        Command cmd("test43");
        cmd.version("1.0.0");
        cmd.about("Test application");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("verbose").short_opt('v').long_opt("verbose").action(ArgAction::SetTrue).help("Enable verbose"));

        std::string help = cmd.render_help();
        if (help.find("test43") == std::string::npos ||
            help.find("1.0.0") == std::string::npos ||
            help.find("verbose") == std::string::npos) {
            std::cerr << "Test 43 failed: render_help\n";
            all_passed = false;
        }
    }

    // render_version
    {
        Command cmd("test44");
        cmd.version("2.5.0");

        std::string version = cmd.render_version();
        if (version.find("test44") == std::string::npos ||
            version.find("2.5.0") == std::string::npos) {
            std::cerr << "Test 44 failed: render_version\n";
            all_passed = false;
        }
    }

    // render_usage
    {
        Command cmd("test45");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.arg(Arg("input").help("Input file"));
        cmd.arg(Arg("output").help("Output file"));
        cmd.arg(Arg("verbose").short_opt('v').long_opt("verbose").action(ArgAction::SetTrue));

        std::string usage = cmd.render_usage();
        if (usage.find("test45") == std::string::npos ||
            usage.find("<input>") == std::string::npos ||
            usage.find("<output>") == std::string::npos ||
            usage.find("[-v") == std::string::npos) {
            std::cerr << "Test 45 failed: render_usage: " << usage << "\n";
            all_passed = false;
        }
    }

    // propagate_version
    {
        Command build_sub("build");
        build_sub.disable_help_flag(true).disable_version_flag(true);

        Command cmd("test46");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.version("3.0.0");
        cmd.propagate_version(true);
        cmd.subcommand(std::move(build_sub));

        if (cmd.get_setting(CommandSetting::PropagateVersion) != true) {
            std::cerr << "Test 46 failed: propagate_version setting not set\n";
            all_passed = false;
        }
    }

    // args_override_self
    {
        Command cmd("test47");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.args_override_self(true);
        cmd.arg(Arg("config").long_opt("config").action(ArgAction::Set).value_name("FILE"));

        std::vector<std::string> args = {"--config=first", "--config=second"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success || result.matches.get_one("config").unwrap() != "second") {
            std::cerr << "Test 47 failed: args_override_self\n";
            all_passed = false;
        }
    }

    // ignore_errors
    {
        Command cmd("test48");
        cmd.disable_help_flag(true).disable_version_flag(true);
        cmd.ignore_errors(true);
        cmd.arg(Arg("config").long_opt("config").action(ArgAction::Set).value_name("FILE"));

        std::vector<std::string> args = {"--config"};
        auto result = cmd.try_get_matches_from(args);
        if (!result.success) {
            std::cerr << "Test 48 failed: ignore_errors should not fail on missing value. Error: ";
            for (const auto& e : result.errors) {
                std::cerr << e << " ";
            }
            std::cerr << "\n";
            all_passed = false;
        }
    }

    if (all_passed) {
        std::cout << "All CLI tests passed!\n";
    }
    return all_passed;
}
