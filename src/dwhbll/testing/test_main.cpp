#include <dwhbll/testing/testing.h>

#include <print>
#include <string_view>
#include <unistd.h>

namespace {

void print_help(const char* prog) {
    std::println("Usage: {} [OPTIONS] [PATTERNS...]", prog);
    std::println("\nTest Runner Options:");
    std::println("  -h, --help                Show this help message");
    std::println("  -v, --verbose             Increase output verbosity");
    std::println("  -q, --quiet               Minimal summary output");
    std::println("  -l, --list                List available test suites and tests");
    std::println("  -x, --fail-fast           Stop on first test failure");
    std::println("  --suite=<name>            Run only test suites matching <name>");
    std::println("  -f, --filter=<pattern>    Filter tests by name or wildcard pattern (*, ?)");
    std::println("  -t, --tag=<expr>          Filter tests by tag expression (e.g. 'foo,~bar')");
    std::println("  --color[=WHEN]            Control color output: 'auto', 'always', or 'never'");
    std::println("\nPositional arguments are treated as test name filter patterns.");
}

} // namespace

int main(int argc, char** argv) {
    dwhbll::test::options opts;
    opts.color = isatty(STDOUT_FILENO);

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbosity = 2;
        } else if (arg == "-q" || arg == "--quiet") {
            opts.verbosity = 0;
        } else if (arg == "-l" || arg == "--list") {
            opts.list_only = true;
        } else if (arg == "-x" || arg == "--fail-fast") {
            opts.fail_fast = true;
        } else if (arg == "--color") {
            opts.color = true;
        } else if (arg == "--color=never") {
            opts.color = false;
        } else if (arg == "--color=always") {
            opts.color = true;
        } else if (arg == "--color=auto") {
            opts.color = isatty(STDOUT_FILENO);
        } else if (arg.starts_with("--suite=")) {
            opts.suite_filter = arg.substr(sizeof("--suite=") - 1);
        } else if (arg == "--suite" && i + 1 < argc) {
            opts.suite_filter = argv[++i];
        } else if (arg.starts_with("--filter=")) {
            opts.patterns.emplace_back(arg.substr(sizeof("--filter=") - 1));
        } else if ((arg == "-f" || arg == "--filter") && i + 1 < argc) {
            opts.patterns.emplace_back(argv[++i]);
        } else if (arg.starts_with("--tag=")) {
            opts.tags = dwhbll::test::parse_filter(arg.substr(sizeof("--tag=") - 1));
        } else if ((arg == "-t" || arg == "--tag") && i + 1 < argc) {
            opts.tags = dwhbll::test::parse_filter(argv[++i]);
        } else if (!arg.starts_with("-")) {
            opts.patterns.emplace_back(arg);
        }
    }

    return dwhbll::test::run_all(opts);
}
