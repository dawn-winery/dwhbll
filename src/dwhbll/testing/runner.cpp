#include <dwhbll/console/ansi_escape.h>
#include <dwhbll/testing/harness.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <print>

namespace dwhbll::test {

namespace {

namespace color = console::ansi_escape::color;

std::string_view status_color(test_status status) {
    switch (status) {
        case test_status::pass: return color::green;
        case test_status::fail: return color::red;
        case test_status::xfail: return color::yellow;
        case test_status::xpass: return color::magenta;
        case test_status::unsupported: return color::yellow;
        case test_status::unresolved: return color::red;
        case test_status::untested: return color::dim;
    }
    return color::reset;
}

std::string get_source_line(std::string_view file_path, std::uint32_t line_num) {
    if (file_path.empty() || line_num == 0)
        return "";
    std::ifstream file{std::string(file_path)};
    if (!file.is_open())
        return "";
    std::string line;
    std::uint32_t current_line = 0;
    while (std::getline(file, line)) {
        if (++current_line == line_num) {
            auto start = line.find_first_not_of(" \t");
            return (start != std::string::npos) ? line.substr(start) : line;
        }
    }
    return "";
}

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

    if (use_color)
        std::println(out, "\n{}=== {} Summary ==={}", color::bold, suite_name, color::reset);
    else
        std::println(out, "\n=== {} Summary ===", suite_name);

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
    if (std::filesystem::exists("/proc/self/exe"))
        log_path = (std::filesystem::read_symlink("/proc/self/exe").parent_path() / "dwhbll_test.log").display_string();

    FILE* console_out = stdout;
    int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log_fd != -1) {
        int orig_stdout = ::dup(STDOUT_FILENO);
        if (orig_stdout != -1) {
            console_out = ::fdopen(orig_stdout, "w");
        }
        ::dup2(log_fd, STDOUT_FILENO);
        ::dup2(log_fd, STDERR_FILENO);
        ::close(log_fd);
    }

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
        return 0;
    }

    auto exec_options = options;
    if (!exec_options.on_test_start) {
        exec_options.on_test_start = [console_out, &exec_options](const test_info& info) {
            std::println("\n=== RUNNING: {} ===", info.name);
            std::fflush(stdout);

            if (exec_options.color) {
                std::print(console_out, "{}{}:{} {}", color::cyan, "RUNNING", color::reset, info.name);
            } else {
                std::print(console_out, "RUNNING: {}", info.name);
            }
            std::fflush(console_out);
        };
    }

    if (!exec_options.on_test_end) {
        exec_options.on_test_end = [console_out, &exec_options](const test_result& tr) {
            auto st_str = to_status_string(tr.status);
            std::println("=== END: {} ({}) ===", tr.name, st_str);
            std::fflush(stdout);

            bool has_reason = !tr.message.empty() &&
                (tr.status == test_status::unsupported ||
                 tr.status == test_status::xfail ||
                 tr.status == test_status::xpass);

            std::size_t running_len = 9 + tr.name.size();
            std::size_t status_len = st_str.size() + 2 + tr.name.size() + (has_reason ? 3 + tr.message.size() : 0);
            std::size_t pad_len = (running_len > status_len) ? (running_len - status_len) : 0;
            std::string pad(pad_len, ' ');

            if (exec_options.color) {
                auto col = status_color(tr.status);
                if (has_reason)
                    std::print(console_out, "\r{}{}:{} {} ({}){}\033[K\n", col, st_str, color::reset, tr.name, tr.message, pad);
                else
                    std::print(console_out, "\r{}{}:{} {}{}\033[K\n", col, st_str, color::reset, tr.name, pad);
            } else {
                if (has_reason)
                    std::print(console_out, "\r{}: {} ({}){}\n", st_str, tr.name, tr.message, pad);
                else
                    std::print(console_out, "\r{}: {}{}\n", st_str, tr.name, pad);
            }

            for (const auto& f : tr.failures) {
                auto src_line = get_source_line(f.loc.file_name(), f.loc.line());
                std::string_view fn_name = f.loc.function_name();
                if (exec_options.color) {
                    if (!fn_name.empty())
                        std::println(console_out, "    {}:{}: in {}:", f.loc.file_name(), f.loc.line(), fn_name);
                    else
                        std::println(console_out, "    {}:{}:", f.loc.file_name(), f.loc.line());
                    if (!src_line.empty())
                        std::println(console_out, "      {:4d} | {}", f.loc.line(), src_line);
                    std::println(console_out, "      {}{}{}", color::red, f.msg, color::reset);
                } else {
                    if (!fn_name.empty())
                        std::println(console_out, "    {}:{}: in {}:", f.loc.file_name(), f.loc.line(), fn_name);
                    else
                        std::println(console_out, "    {}:{}:", f.loc.file_name(), f.loc.line());
                    if (!src_line.empty())
                        std::println(console_out, "      {:4d} | {}", f.loc.line(), src_line);
                    std::println(console_out, "      {}", f.msg);
                }
            }
            std::fflush(console_out);
        };
    }

    summary_counts total;
    auto suite_results = run_suites(exec_options);

    for (const auto& suite_res : suite_results) {
        total += suite_res.counts;
        print_summary_block(console_out, suite_res.suite_name, suite_res.counts, exec_options.color);
    }

    if (suite_results.size() > 1)
        print_summary_block(console_out, "Test", total, exec_options.color);

    return total.is_success() ? 0 : 1;
}

} // namespace dwhbll::test
