#include <dwhbll/testing/testing.h>
#include <dwhbll/cli/command.h>

#include <unistd.h>

int main(int argc, char** argv) {
    using namespace dwhbll::cli;

    Command cmd("dwhbll_test");
    cmd.about("dwhbll test runner");
    cmd.arg(Arg("list")
        .short_opt('l')
        .long_opt("list")
        .help("List available test suites and tests")
        .action(ArgAction::SetTrue));
    cmd.arg(Arg("fail-fast")
        .short_opt('x')
        .long_opt("fail-fast")
        .help("Stop on first test failure")
        .action(ArgAction::SetTrue));
    cmd.arg(Arg("suite")
        .long_opt("suite")
        .help("Run only test suites matching <name>")
        .action(ArgAction::Set)
        .value_name("NAME"));
    cmd.arg(Arg("filter")
        .short_opt('f')
        .long_opt("filter")
        .help("Filter tests by name or wildcard pattern (*, ?)")
        .action(ArgAction::Append)
        .value_name("PATTERN"));
    cmd.arg(Arg("color")
        .long_opt("color")
        .help("Control color output: 'auto', 'always', or 'never'")
        .action(ArgAction::Set)
        .value_name("WHEN")
        .default_missing_value("always"));
    cmd.arg(Arg("patterns")
        .help("Test name filter patterns")
        .action(ArgAction::Set)
        .num_args(ValueRange::any()));

    auto matches = cmd.get_matches(argc, argv);

    dwhbll::test::options opts;
    opts.color = isatty(STDOUT_FILENO);
    opts.list_only = matches.get_flag("list");
    opts.fail_fast = matches.get_flag("fail-fast");

    if (matches.contains_id("suite")) {
        opts.suite_filter = matches.get_one("suite").unwrap();
    }
    if (matches.contains_id("color")) {
        auto val = matches.get_one("color").unwrap();
        if (val == "never" || val == "false")
            opts.color = false;
        else if (val == "always" || val == "true")
            opts.color = true;
        else if (val == "auto")
            opts.color = isatty(STDOUT_FILENO);
    }

    opts.patterns = matches.get_many("filter");
    auto pos_patterns = matches.get_many("patterns");
    opts.patterns.insert(opts.patterns.end(), pos_patterns.begin(), pos_patterns.end());

    return dwhbll::test::run_all(opts);
}
