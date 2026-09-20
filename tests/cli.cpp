#include <dwhbll/cli/command.h>
#include <dwhbll/testing/testing.h>

#include <iostream>
#include <vector>

using namespace dwhbll::cli;
using namespace dwhbll::test;

namespace cli {

[[=test]]
void option_parsing()
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
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("config").unwrap(), "config.txt");
}

[[=test]]
void flag_parsing()
{
    Command cmd("test2");
    cmd.arg(Arg("verbose")
        .short_opt('v')
        .long_opt("verbose")
        .help("Enable verbose output")
        .action(ArgAction::SetTrue));

    std::vector<std::string> args = {"-v"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE(result.matches.get_flag("verbose"));
}

[[=test]]
void default_values()
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
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("count").unwrap(), "10");
}

[[=test]]
void positional_arguments()
{
    Command cmd("test4");
    cmd.arg(Arg("input").help("Input file"));
    cmd.arg(Arg("output").help("Output file"));

    std::vector<std::string> args = {"input.txt", "output.txt"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("input").unwrap(), "input.txt");
    REQUIRE_EQ(result.matches.get_one("output").unwrap(), "output.txt");
}

[[=test]]
void long_option_with_equals()
{
    Command cmd("test5");
    cmd.arg(Arg("config")
        .long_opt("config")
        .help("Configuration file")
        .action(ArgAction::Set)
        .value_name("FILE"));

    std::vector<std::string> args = {"--config=config.txt"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("config").unwrap(), "config.txt");
}

[[=test]]
void short_options_combined()
{
    Command cmd("test6");
    cmd.arg(Arg("a").short_opt('a').action(ArgAction::SetTrue));
    cmd.arg(Arg("b").short_opt('b').action(ArgAction::SetTrue));
    cmd.arg(Arg("c").short_opt('c').action(ArgAction::SetTrue));

    std::vector<std::string> args = {"-abc"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE(result.matches.get_flag("a"));
    REQUIRE(result.matches.get_flag("b"));
    REQUIRE(result.matches.get_flag("c"));
}

[[=test]]
void value_delimiter()
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
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_many("items").size(), 3);
}

[[=test]]
void count_action()
{
    Command cmd("test8");
    cmd.arg(Arg("verbose")
        .short_opt('v')
        .long_opt("verbose")
        .help("Verbosity level")
        .action(ArgAction::Count));

    std::vector<std::string> args = {"-vvv"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.count("verbose"), 3);
}

[[=test]]
void append_action()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("items")
        .long_opt("items")
        .action(ArgAction::Append)
        .value_name("ITEM"));

    std::vector<std::string> args = {"--items=a", "--items=b", "--items=c"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_many("items").size(), 3);

    auto values = result.matches.get_many("items");
    REQUIRE_EQ(values[0], "a");
    REQUIRE_EQ(values[1], "b");
    REQUIRE_EQ(values[2], "c");
}

[[=test]]
void set_false_action()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("no_color")
        .long_opt("no-color")
        .action(ArgAction::SetFalse)
        .help("Disable color output"));

    std::vector<std::string> args = {"--no-color"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_flag("no_color"), false);
}

[[=test]]
void trailing_var_arg()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("cmd").help("Command to run"));
    cmd.arg(Arg("args").help("Arguments").trailing_var_arg(true));

    std::vector<std::string> args = {"mycmd", "arg1", "arg2", "arg3"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("cmd").unwrap(), "mycmd");
    REQUIRE_EQ(result.matches.get_many("args").size(), 3);
}

[[=test]]
void num_args_validation()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("files")
        .long_opt("files")
        .action(ArgAction::Set)
        .num_args(ValueRange::at_least(2))
        .value_name("FILE"));

    std::vector<std::string> args = {"--files", "a.txt"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);

    std::vector<std::string> args2 = {"--files", "a.txt", "b.txt"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_many("files").size(), 2);
}

[[=test]]
void num_args_with_value_delimiter()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("items")
        .long_opt("items")
        .action(ArgAction::Set)
        .value_delimiter(',')
        .num_args(ValueRange::fixed(3))
        .value_name("ITEM"));

    std::vector<std::string> args = {"--items=a,b"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);

    std::vector<std::string> args2 = {"--items=a,b,c"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_many("items").size(), 3);
}

[[=test]]
void subcommands()
{
    Command cmd("test9");
    cmd.subcommand(Command("sub1")
        .arg(Arg("arg1").help("Sub arg 1")));

    std::vector<std::string> args = {"sub1", "value1"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);

    auto sub_name = result.matches.subcommand_name();
    REQUIRE(sub_name.is_some());
    REQUIRE_EQ(sub_name.unwrap(), "sub1");

    const ArgMatches* sub_matches = result.matches.subcommand_matches("sub1");
    REQUIRE(sub_matches);
    REQUIRE_EQ(sub_matches->get_one("arg1").unwrap(), "value1");
}

[[=test]]
void subcommand_with_options()
{
    Command build_sub("build");
    build_sub.disable_help_flag(true).disable_version_flag(true);
    build_sub.arg(Arg("target").short_opt('t').long_opt("target").action(ArgAction::Set).value_name("TARGET"));
    build_sub.arg(Arg("release").short_opt('r').long_opt("release").action(ArgAction::SetTrue));

    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.subcommand(std::move(build_sub));

    std::vector<std::string> args = {"build", "--target", "x86_64", "--release"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    const ArgMatches* sub_matches = result.matches.subcommand_matches("build");
    REQUIRE(sub_matches);
    REQUIRE_EQ(sub_matches->get_one("target").unwrap(), "x86_64");
    REQUIRE(sub_matches->get_flag("release"));
}

[[=test]]
void required_argument_missing()
{
    Command cmd("test");
    cmd.arg(Arg("required_arg")
        .long_opt("required")
        .help("Required argument")
        .action(ArgAction::Set)
        .required(true));

    std::vector<std::string> args = {};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
}

[[=test]]
void literals()
{
    using namespace dwhbll::cli::literals;
    Arg a = "test_literal2"_Arg;
    REQUIRE_EQ(a.id(), "test_literal2");
}

[[=test]]
void subcommand_help()
{
    Command cmd("test");
    cmd.disable_help_flag(false).disable_version_flag(true);
    cmd.subcommand(Command("build")
        .disable_help_flag(false)
        .disable_version_flag(true)
        .arg(Arg("target").short_opt('t').long_opt("target").action(ArgAction::Set).value_name("TARGET")));

    std::vector<std::string> args = {"build", "--help"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
}

[[=test]]
void conflicts_validation()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue).conflicts_with("opt2"));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));

    std::vector<std::string> args = {"--opt1", "--opt2"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
    REQUIRE_NE(result.errors[0].find("conflicts"), std::string::npos);
}

[[=test]]
void requires_validation()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::Set).requires_arg("opt2"));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::Set));

    std::vector<std::string> args = {"--opt1", "value1"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
    REQUIRE_NE(result.errors[0].find("requires"), std::string::npos);
}

[[=test]]
void requires_with_both_present()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::Set).requires_arg("opt2"));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::Set));

    std::vector<std::string> args = {"--opt1", "value1", "--opt2", "value2"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void conflicts_with_both_not_present()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue).conflicts_with("opt2"));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));

    std::vector<std::string> args = {};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void environment_variable_support()
{
    setenv("TEST_CLI_ENV_VAR", "env_value", 1);

    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("from_env").long_opt("from-env").action(ArgAction::Set).env("TEST_CLI_ENV_VAR"));

    std::vector<std::string> args = {};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("from_env").unwrap(), "env_value");

    unsetenv("TEST_CLI_ENV_VAR");
    Command cmd2("test17b");
    cmd2.disable_help_flag(true).disable_version_flag(true);
    cmd2.arg(Arg("from_env_default").long_opt("from-env-default").action(ArgAction::Set)
        .env("NONEXISTENT_VAR", dwhbll::stl_ext::Option<std::string>(std::string("default_value"))));

    std::vector<std::string> args2 = {};
    auto result2 = cmd2.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_one("from_env_default").unwrap(), "default_value");

    unsetenv("TEST_CLI_ENV_VAR");
}

[[=test]]
void arg_group()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt3").long_opt("opt3").action(ArgAction::SetTrue));
    cmd.group(ArgGroup("group1")
        .args({"opt1", "opt2"})
        .required(true));

    std::vector<std::string> args = {"--opt1"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void arg_group_multiple()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
    cmd.group(ArgGroup("group1")
        .args({"opt1", "opt2"})
        .multiple(true));

    std::vector<std::string> args = {"--opt1", "--opt2"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void arg_group_conflict()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt3").long_opt("opt3").action(ArgAction::SetTrue));
    cmd.group(ArgGroup("group1")
        .args({"opt1", "opt2"})
        .conflicts_with("opt3"));

    std::vector<std::string> args = {"--opt1", "--opt3"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
}

[[=test]]
void allow_hyphen_values()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("pattern")
        .long_opt("pattern")
        .action(ArgAction::Set)
        .allow_hyphen_values(true)
        .value_name("PATTERN"));

    std::vector<std::string> args = {"--pattern", "-file.txt"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("pattern").unwrap(), "-file.txt");
}

[[=test]]
void allow_negative_numbers()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("num")
        .long_opt("num")
        .action(ArgAction::Set)
        .allow_negative_numbers(true)
        .value_name("NUM"));

    std::vector<std::string> args = {"--num", "-42"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("num").unwrap(), "-42");
}

[[=test]]
void require_equals()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("config")
        .long_opt("config")
        .action(ArgAction::Set)
        .require_equals(true)
        .value_name("FILE"));

    std::vector<std::string> args = {"--config=config.txt"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("config").unwrap(), "config.txt");

    std::vector<std::string> args2 = {"--config", "config.txt"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(!result2.success);
}

[[=test]]
void value_terminator()
{
    Command cmd("test");
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
    REQUIRE(result.success);

    auto cmds = result.matches.get_many("cmds");
    REQUIRE_EQ(cmds.size(), 3);
    REQUIRE_EQ(cmds[0], "find");
    REQUIRE_EQ(cmds[1], "--type");
    REQUIRE_EQ(cmds[2], "f");
    REQUIRE_EQ(result.matches.get_one("location").unwrap(), "/home");
}

[[=test]]
void raw()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("args")
        .long_opt("args")
        .raw(true)
        .value_name("ARGS"));

    std::vector<std::string> args = {"--args", "-v", "-v", "--flag", "value"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    auto args_vec = result.matches.get_many("args");
    REQUIRE_EQ(args_vec.size(), 4);
}

[[=test]]
void ignore_case()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("mode")
        .long_opt("mode")
        .action(ArgAction::Set)
        .value_parser({"FAST", "SLOW"}));

    std::vector<std::string> args = {"--mode", "FAST"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("mode").unwrap(), "FAST");

    std::vector<std::string> args2 = {"--mode", "SLOW"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_one("mode").unwrap(), "SLOW");
}

[[=test]]
void default_value_if()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("flag").long_opt("flag").action(ArgAction::SetTrue));
    cmd.arg(Arg("value")
        .long_opt("value")
        .action(ArgAction::Set)
        .default_value_if(ArgPredicate::equals("flag", "true"), "conditional_default"));

    std::vector<std::string> args = {"--flag", "--value", "explicit"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("value").unwrap(), "explicit");

    std::vector<std::string> args2 = {"--flag"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_one("value").unwrap(), "conditional_default");
}

[[=test]]
void default_value_unless()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("debug").long_opt("debug").action(ArgAction::SetTrue));
    cmd.arg(Arg("log_level")
        .long_opt("log-level")
        .action(ArgAction::Set)
        .default_value_unless(ArgPredicate::is_present("debug"), "debug"));

    std::vector<std::string> args = {"--debug"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE(!result.matches.contains_id("log_level"));

    std::vector<std::string> args2 = {};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_one("log_level").unwrap(), "debug");
}

[[=test]]
void value_parser()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("port")
        .long_opt("port")
        .action(ArgAction::Set)
        .value_parser(value_parser_uint())
        .value_name("PORT"));

    std::vector<std::string> args = {"--port", "8080"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("port").unwrap(), "8080");

    std::vector<std::string> args2 = {"--port", "invalid"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(!result2.success);
}

[[=test]]
void value_parser_bool()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("verbose")
        .long_opt("verbose")
        .action(ArgAction::Set)
        .value_parser(dwhbll::cli::value_parser_bool()));

    std::vector<std::string> args = {"--verbose", "true"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("verbose").unwrap(), "true");

    std::vector<std::string> args2 = {"--verbose", "false"};
    auto result2 = cmd.try_get_matches_from(args2);
    REQUIRE(result2.success);
    REQUIRE_EQ(result2.matches.get_one("verbose").unwrap(), "false");
}

[[=test]]
void subcommand_alias()
{
    Command build_sub("build");
    build_sub.disable_help_flag(true).disable_version_flag(true);
    build_sub.alias("b");

    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.subcommand(std::move(build_sub));

    std::vector<std::string> args = {"b"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void command_settings()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.no_binary_name(true);
    cmd.arg(Arg("cmd").help("Command"));
    cmd.arg(Arg("args").help("Args").trailing_var_arg(true));

    std::vector<std::string> args = {"mycmd", "arg1", "arg2"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("cmd").unwrap(), "mycmd");
}

[[=test]]
void external_subcommand()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.external_subcommand(true);

    std::vector<std::string> args = {"unknown_cmd", "arg1", "arg2"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("external_subcommand").unwrap(), "unknown_cmd");
}

[[=test]]
void mut_arg()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("verbose").short_opt('v').action(ArgAction::SetTrue));
    cmd.mut_arg("verbose", [](Arg a) { return a.short_opt('V'); });

    std::vector<std::string> args = {"-V"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE(result.matches.get_flag("verbose"));
}

[[=test]]
void mut_args()
{
    Command cmd("test");
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
    REQUIRE(result.success);
}

[[=test]]
void mut_group()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue));
    cmd.group(ArgGroup("mygroup").args({"opt1", "opt2"}));
    cmd.mut_group("mygroup", [](ArgGroup g) { return g.required(true); });

    std::vector<std::string> args = {};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
}

[[=test]]
void mut_subcommand()
{
    Command build_sub("build");
    build_sub.disable_help_flag(true).disable_version_flag(true);
    build_sub.arg(Arg("target").long_opt("target").action(ArgAction::Set));

    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.subcommand(std::move(build_sub));
    cmd.mut_subcommand("build", [](Command c) {
        return c.arg(Arg("release").long_opt("release").action(ArgAction::SetTrue));
    });

    std::vector<std::string> args = {"build", "--release"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void mut_subcommands()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.subcommand(Command("sub1"));
    cmd.subcommand(Command("sub2"));
    cmd.mut_subcommands([](Command c) {
        return c.disable_help_flag(true);
    });

    std::vector<std::string> args = {"sub1", "--help"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(!result.success);
}

[[=test]]
void arg_display_order()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue).display_order(1));
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue).display_order(0));

    std::string help = cmd.render_help();
    size_t pos1 = help.find("--opt1");
    size_t pos2 = help.find("--opt2");
    REQUIRE_LE(pos1, pos2);
}

[[=test]]
void arg_next_line_help()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt")
        .long_opt("opt")
        .action(ArgAction::Set)
        .value_name("VALUE")
        .next_line_help(true)
        .help("This is a very long help message that should appear on the next line"));

    std::string help = cmd.render_help();
    REQUIRE_NE(help.find("This is a very long"), std::string::npos);
}

[[=test]]
void arg_hide_short_help()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("opt1").long_opt("opt1").action(ArgAction::SetTrue));
    cmd.arg(Arg("opt2").long_opt("opt2").action(ArgAction::SetTrue).hide_short_help(true));

    std::string help = cmd.render_help();
    REQUIRE_NE(help.find("--opt2"), std::string::npos);
}

[[=test]]
void arg_hide_possible_values()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("mode")
        .long_opt("mode")
        .action(ArgAction::Set)
        .value_parser({"fast", "slow"})
        .hide_possible_values(true));

    std::vector<std::string> args = {"--mode", "fast"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

[[=test]]
void render_help()
{
    Command cmd("test");
    cmd.version("1.0.0");
    cmd.about("Test application");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("verbose").short_opt('v').long_opt("verbose").action(ArgAction::SetTrue).help("Enable verbose"));

    std::string help = cmd.render_help();
    REQUIRE_NE(help.find("test"), std::string::npos);
    REQUIRE_NE(help.find("1.0.0"), std::string::npos);
    REQUIRE_NE(help.find("verbose"), std::string::npos);
}

[[=test]]
void render_version()
{
    Command cmd("test");
    cmd.version("2.5.0");

    std::string version = cmd.render_version();
    REQUIRE_NE(version.find("test"), std::string::npos);
    REQUIRE_NE(version.find("2.5.0"), std::string::npos);
}

[[=test]]
void render_usage()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.arg(Arg("input").help("Input file"));
    cmd.arg(Arg("output").help("Output file"));
    cmd.arg(Arg("verbose").short_opt('v').long_opt("verbose").action(ArgAction::SetTrue));

    std::string usage = cmd.render_usage();
    REQUIRE_NE(usage.find("test"), std::string::npos);
    REQUIRE_NE(usage.find("<input>"), std::string::npos);
    REQUIRE_NE(usage.find("<output>"), std::string::npos);
    REQUIRE_NE(usage.find("[-v"), std::string::npos);
}

[[=test]]
void propagate_version()
{
    Command build_sub("build");
    build_sub.disable_help_flag(true).disable_version_flag(true);

    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.version("3.0.0");
    cmd.propagate_version(true);
    cmd.subcommand(std::move(build_sub));

    REQUIRE(cmd.get_setting(CommandSetting::PropagateVersion));
}

[[=test]]
void args_override_self()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.args_override_self(true);
    cmd.arg(Arg("config").long_opt("config").action(ArgAction::Set).value_name("FILE"));

    std::vector<std::string> args = {"--config=first", "--config=second"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
    REQUIRE_EQ(result.matches.get_one("config").unwrap(), "second");
}

[[=test]]
void ignore_errors()
{
    Command cmd("test");
    cmd.disable_help_flag(true).disable_version_flag(true);
    cmd.ignore_errors(true);
    cmd.arg(Arg("config").long_opt("config").action(ArgAction::Set).value_name("FILE"));

    std::vector<std::string> args = {"--config"};
    auto result = cmd.try_get_matches_from(args);
    REQUIRE(result.success);
}

}

TEST_REGISTER_FILE();
