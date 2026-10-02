#include <dwhbll/console/ansi_escape.h>
#include <dwhbll/testing/harness.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <print>

namespace dwhbll::test {

namespace {

namespace color = console::ansi_escape::color;

void print_summary_block(FILE* out, std::string_view suite_name,
                         const summary_counts& counts, bool use_color) {
    auto print_line = [&](std::string_view label, std::size_t count,
                          std::string_view col, bool hide_if_zero = false) {
        if (hide_if_zero && count == 0)
            return;
        std::string full_label = std::format("# of {}", label);
        if (use_color && !col.empty() && count > 0)
            std::println(out, "  {:<26} {}{}{}", full_label, col, count, color::reset);
        else
            std::println(out, "  {:<26} {}", full_label, count);
    };

    auto bold = use_color ? color::bold : "";
    auto reset = use_color ? color::reset : "";
    std::println(out, "\n{}=== {} Summary ==={}", bold, suite_name, reset);

    print_line("expected passes", counts.passes, "");
    print_line("unexpected failures", counts.failures, color::red);
    print_line("expected failures", counts.xfails, color::yellow, true);
    print_line("unexpected successes", counts.xpasses, color::magenta, true);
    print_line("unsupported tests", counts.unsupported, color::yellow, true);
    print_line("unresolved testcases", counts.unresolved, color::red, true);
    print_line("untested testcases", counts.untested, "", true);
    std::fflush(out);
}

} // namespace

runner::runner(bool def_harness) {
    if (def_harness)
        harnesses_.push_back(std::make_shared<default_harness>());
}

runner& runner::add_harness(std::shared_ptr<test_harness> harness) {
    if (harness)
        harnesses_.push_back(std::move(harness));
    return *this;
}

std::vector<test_info> runner::list_all_tests() const {
    std::vector<test_info> all;
    for (const auto& h : harnesses_) {
        auto tests = h->list_tests();
        all.insert(all.end(), tests.begin(), tests.end());
    }
    return all;
}

std::vector<suite_result> runner::run_suites(const options& options) const {
    std::vector<suite_result> suite_results;
    for (const auto& h : harnesses_) {
        if (!options.suite_filter.empty() && h->name() != options.suite_filter)
            continue;
        suite_results.push_back(h->run(options));
    }
    return suite_results;
}

int runner::run(const options& options) const {
    std::string log_path = "dwhbll_test.log";
    std::error_code ec;
    if (std::filesystem::exists("/proc/self/exe", ec)) {
        auto exe_path = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec) {
#if __cpp_lib_format_path >= 202506L
            log_path = (exe_path.parent_path() / "dwhbll_test.log").display_string();
#else
            log_path = (exe_path.parent_path() / "dwhbll_test.log").string();
#endif
        }
    }

    FILE* console_out = options.console_out ? options.console_out : stdout;
    FILE* duped_console_out = nullptr;

    int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log_fd != -1) {
        int orig_stdout = ::dup(STDOUT_FILENO);
        if (orig_stdout != -1) {
            duped_console_out = ::fdopen(orig_stdout, "w");
            if (duped_console_out)
                console_out = duped_console_out;
            else
                ::close(orig_stdout);
        }
        ::dup2(log_fd, STDOUT_FILENO);
        ::dup2(log_fd, STDERR_FILENO);
        ::close(log_fd);
    }

    auto cleanup = [&]() {
        std::fflush(stdout);
        std::fflush(stderr);
        if (duped_console_out) {
            std::fflush(duped_console_out);
            ::fclose(duped_console_out);
        }
    };

    if (options.list_only) {
        auto all_tests = list_all_tests();
        std::println(console_out, "Available tests ({} total):", all_tests.size());
        for (const auto& t : all_tests) {
            std::string extra;
            if (t.is_skip)
                extra = " [skip]";
            if (t.is_xfail)
                extra = " [xfail]";
            std::println(console_out, "  {}:{}{}", t.suite, t.name, extra);
        }
        std::fflush(console_out);
        cleanup();
        return 0;
    }

    auto exec_options = options;
    exec_options.console_out = console_out;

    summary_counts total;
    auto suite_results = run_suites(exec_options);

    for (const auto& suite_res : suite_results) {
        total += suite_res.counts;
        print_summary_block(console_out, suite_res.suite_name, suite_res.counts, options.color);
    }

    if (suite_results.size() > 1)
        print_summary_block(console_out, "Test", total, options.color);

    cleanup();
    return total.is_success() ? 0 : 1;
}

} // namespace dwhbll::test
