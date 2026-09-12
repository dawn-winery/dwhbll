#include <dwhbll/bench/bench.h>

#include <print>
#include <unistd.h>

namespace {

void print_help(const char* prog) {
    std::println("Usage: {} [OPTIONS] [PATTERNS...]", prog);
    std::println("\nBenchmark Runner Options:");
    std::println("  -h, --help                Show this help message");
    std::println("  -l, --list                List available benchmarks");
    std::println("  --iters=<n>               Iterations per section (default 1000)");
    std::println("  --warmup-iters=<n>        Warmup iterations per section (default 3)");
    std::println("  --color[=WHEN]            Control color output: 'auto', 'always', or 'never'");
}

} // namespace

int main(int argc, char** argv) {
    dwhbll::bench::options opts;
    opts.color = isatty(STDOUT_FILENO);

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-l" || arg == "--list") {
            opts.list_only = true;
        } else if (arg.starts_with("--iters=")) {
            opts.iterations = std::stoul(std::string(arg.substr(sizeof("--iters=") - 1)));
        } else if (arg.starts_with("--warmup-iters=")) {
            opts.warmup_iterations = std::stoul(std::string(arg.substr(sizeof("--warmup-iters=") - 1)));
        } else if (arg == "--color") {
            opts.color = true;
        } else if (arg == "--color=never") {
            opts.color = false;
        } else if (arg == "--color=always") {
            opts.color = true;
        } else if (arg == "--color=auto") {
            opts.color = isatty(STDOUT_FILENO);
        } else if (!arg.starts_with("-")) {
            opts.patterns.emplace_back(arg);
        }
    }

    return dwhbll::bench::run_all(opts);
}
