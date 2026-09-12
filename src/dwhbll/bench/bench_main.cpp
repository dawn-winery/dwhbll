#include <dwhbll/bench/bench.h>
#include <dwhbll/cli/command.h>

#include <unistd.h>

int main(int argc, char** argv) {
    using namespace dwhbll::cli;

    Command cmd("dwhbll_bench");
    cmd.about("dwhbll benchmark runner");
    cmd.arg(Arg("list")
        .short_opt('l')
        .long_opt("list")
        .help("List available benchmarks")
        .action(ArgAction::SetTrue));
    cmd.arg(Arg("iters")
        .long_opt("iters")
        .help("Iterations per section (default 1000)")
        .action(ArgAction::Set)
        .value_name("N"));
    cmd.arg(Arg("warmup-iters")
        .long_opt("warmup-iters")
        .help("Warmup iterations per section (default 3)")
        .action(ArgAction::Set)
        .value_name("N"));
    cmd.arg(Arg("color")
        .long_opt("color")
        .help("Control color output: 'auto', 'always', or 'never'")
        .action(ArgAction::Set)
        .value_name("WHEN")
        .default_missing_value("always"));
    cmd.arg(Arg("patterns")
        .help("Benchmark name filter patterns")
        .action(ArgAction::Set)
        .num_args(ValueRange::any()));

    auto matches = cmd.get_matches(argc, argv);

    dwhbll::bench::options opts;
    opts.color = isatty(STDOUT_FILENO);
    opts.list_only = matches.get_flag("list");

    if (matches.contains_id("iters")) {
        opts.iterations = std::stoul(matches.get_one("iters").unwrap());
    }
    if (matches.contains_id("warmup-iters")) {
        opts.warmup_iterations = std::stoul(matches.get_one("warmup-iters").unwrap());
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

    opts.patterns = matches.get_many("patterns");

    return dwhbll::bench::run_all(opts);
}
