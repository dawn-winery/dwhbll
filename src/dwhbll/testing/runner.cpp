#include <dwhbll/testing/runner.h>
#include <dwhbll/testing/harness.h>

#include <fstream>
#include <print>

namespace dwhbll::test {

namespace {

namespace color {
    constexpr std::string_view reset   = "\e[0m";
    constexpr std::string_view bold    = "\e[1m";
    constexpr std::string_view dim     = "\e[2m";
    constexpr std::string_view red     = "\e[31m";
    constexpr std::string_view green   = "\e[32m";
    constexpr std::string_view yellow  = "\e[33m";
    constexpr std::string_view blue    = "\e[34m";
    constexpr std::string_view magenta = "\e[35m";
    constexpr std::string_view cyan    = "\e[36m";
}

std::string_view status_color(test_status status) {
    switch (status) {
        case test_status::pass:
            return color::green;
        case test_status::fail:
            return color::red;
        case test_status::xfail:
            return color::yellow;
        case test_status::xpass:
            return color::magenta;
        case test_status::unsupported:
            return color::yellow;
        case test_status::unresolved:
            return color::red;
        case test_status::untested:
            return color::dim;
    }
    return color::reset;
}

std::string format_tags(const std::vector<std::string_view>& tags, bool use_color) {
    if (tags.empty())
        return "";
    std::string s = " ";
    if (use_color)
        s += color::dim;
    s += "[";
    for (std::size_t i = 0; i < tags.size(); ++i) {
        if (i) s += ", ";
        s += tags[i];
    }
    s += "]";
    if (use_color)
        s += color::reset;
    return s;
}

std::string get_source_line(std::string_view file_path, std::uint32_t line_num) {
    if (file_path.empty() || line_num == 0)
        return "";
    std::string path_str(file_path);
    std::ifstream file(path_str);
    if (!file.is_open())
        return "";
    std::string line;
    std::uint32_t current_line = 0;
    while (std::getline(file, line)) {
        ++current_line;
        if (current_line == line_num) {
            auto start = line.find_first_not_of(" \t");
            if (start != std::string::npos)
                return line.substr(start);
            return line;
        }
    }
    return "";
}

void print_summary_block(std::ostream* out, std::string_view suite_name,
                         const summary_counts& counts, bool use_color) {
    auto print_line = [&](std::string_view label, std::size_t count, std::string_view col) {
        if (count == 0 && (label.contains("unexpected") || label.contains("unresolved")
                    || label.contains("untested")))
            return;
        if (out) {
            *out << "# of " << label;
            if (label.size() < 24) *out << "\t";
            *out << "" << count << "\n";
        } else {
            if (use_color && !col.empty()) {
                std::println("{}# of {}{}\t\t{}{}{}", color::dim, label, color::reset,
                        col, count, color::reset);
            } else {
                std::println("# of {}\t\t{}", label, count);
            }
        }
    };

    if (out) {
        *out << "\n=== " << suite_name << " Summary ===\n\n";
    } else {
        if (use_color)
            std::println("\n{}=== {} Summary ==={}", color::bold, suite_name, color::reset);
        else
            std::println("\n=== {} Summary ===", suite_name);
    }

    print_line("expected passes", counts.passes, color::green);
    print_line("unexpected failures", counts.failures, color::red);
    print_line("expected failures", counts.xfails, color::yellow);
    print_line("unexpected successes", counts.xpasses, color::magenta);
    print_line("unsupported tests", counts.unsupported, color::yellow);
    print_line("unresolved testcases", counts.unresolved, color::red);
    print_line("untested testcases", counts.untested, color::dim);
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
    if (options.list_only) {
        auto all_tests = list_all_tests();
        std::println("Available tests ({} total):", all_tests.size());
        for (const auto& t : all_tests) {
            std::string extra;
            if (t.is_skip) extra = " [skip]";
            if (t.is_xfail) extra = " [xfail]";
            std::println("  {}:{} {}{}", t.suite, t.name, format_tags(t.tags, options.color), extra);
        }
        return 0;
    }

    summary_counts total;
    auto suite_results = run_suites(options);

    for (const auto& suite_res : suite_results) {
        total += suite_res.counts;

        for (const auto& tr : suite_res.results) {
            auto st_str = to_status_string(tr.status);
            std::string tags_str = format_tags(tr.tags, options.color);
            std::string plain_tags = format_tags(tr.tags, false);

            if (options.verbosity > 0 || tr.failed()) {
                if (options.color) {
                    auto col = status_color(tr.status);
                    if (!tr.message.empty() && (tr.status == test_status::unsupported
                                || tr.status == test_status::xfail))
                        std::println("{}{}:{} {} ({}){}", col, st_str, color::reset, 
                                tr.name, tr.message, tags_str);
                    else
                        std::println("{}{}:{} {}{}", col, st_str, color::reset,
                                tr.name, tags_str);
                } else {
                    if (!tr.message.empty() && (tr.status == test_status::unsupported
                                || tr.status == test_status::xfail))
                        std::println("{}: {} ({}){}", st_str, tr.name, tr.message, tags_str);
                    else
                        std::println("{}: {}{}", st_str, tr.name, tags_str);
                }

                if (!tr.failures.empty()) {
                    for (const auto& f : tr.failures) {
                        auto src_line = get_source_line(f.loc.file_name(), f.loc.line());
                        std::string_view fn_name = f.loc.function_name();
                        if (options.color) {
                            if (!fn_name.empty()) {
                                std::println("    {}{}:{}:{} in {}{}{}:", color::dim, 
                                        f.loc.file_name(), f.loc.line(),
                                        color::reset,color::cyan, fn_name, color::reset);
                            } else
                                std::println("    {}{}:{}:{}", color::dim, f.loc.file_name(),
                                        f.loc.line(), color::reset);
                            if (!src_line.empty())
                                std::println("      {}{:4d} |{} {}", color::dim,
                                        f.loc.line(), color::reset, src_line);
                            std::println("      {}{}{}", color::red, f.msg, color::reset);
                        } else {
                            if (!fn_name.empty())
                                std::println("    {}:{}: in {}:", f.loc.file_name(),
                                        f.loc.line(), fn_name);
                            else
                                std::println("    {}:{}:", f.loc.file_name(), f.loc.line());
                            if (!src_line.empty())
                                std::println("      {:4d} | {}", f.loc.line(), src_line);
                            std::println("      {}", f.msg);
                        }
                    }
                }
            }
        }

        if (options.verbosity > 0)
            print_summary_block(nullptr, suite_res.suite_name, suite_res.counts, options.color);
    }

    if (suite_results.size() > 1 && options.verbosity > 0)
        print_summary_block(nullptr, "Test", total, options.color);

    return total.is_success() ? 0 : 1;
}

} // namespace dwhbll::test
