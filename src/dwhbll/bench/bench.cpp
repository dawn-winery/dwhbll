#include <dwhbll/bench/bench.h>

#include <cmath>
#include <mutex>
#include <numeric>
#include <print>
#include <string_view>
#include <vector>

#include <dwhbll/debug/debug.h>

namespace dwhbll::bench {

namespace detail {

std::vector<entry>& registry() {
    static std::vector<entry> r;
    return r;
}

struct run_config {
    std::size_t iterations;
    std::size_t warmup_iterations;
};

static std::vector<section_result>* current_sections = nullptr;
static const run_config* current_config = nullptr;
static std::mutex current_sections_mutex;

static const run_config& active_config() {
    static const run_config defaults{1000, 3};
    return current_config ? *current_config : defaults;
}

section::section(std::string_view name, std::source_location loc)
    : name_(name), loc_(loc) {
    const run_config& cfg = active_config();
    total_iterations_ = cfg.iterations;
    warmup_iterations_ = cfg.warmup_iterations;
}

bool section::next() {
    using clock = std::chrono::steady_clock;

    if (state_ == state::warmup) {
        if (warmup_done_ < warmup_iterations_) {
            ++warmup_done_;
            return true;
        }
        state_ = state::measure;
        samples_.reserve(total_iterations_);
        start_ = clock::now();
        return true;
    }

    const auto now = clock::now();
    const double elapsed_ns = std::chrono::duration<double, std::nano>(now - start_).count();
    samples_.push_back(elapsed_ns);
    ++measure_done_;

    if (measure_done_ >= total_iterations_) {
        finalize();
        return false;
    }

    start_ = clock::now();
    return true;
}

void section::finalize() {
    ASSERT(current_sections);

    section_result res;
    res.name = name_.empty() ? std::format("<line {}>", loc_.line()) : std::string(name_);
    res.loc = loc_;
    res.st = compute_stats(std::move(samples_));

    std::lock_guard lock(current_sections_mutex);
    current_sections->push_back(std::move(res));
}

stats section::compute_stats(std::vector<double>&& samples) {
    stats s;
    s.iterations = samples.size();
    if (samples.empty())
        return s;

    std::sort(samples.begin(), samples.end());
    s.min_ns = samples.front();
    s.max_ns = samples.back();

    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    s.mean_ns = sum / static_cast<double>(samples.size());

    const std::size_t mid = samples.size() / 2;
    s.median_ns = (samples.size() % 2 == 0)
        ? (samples[mid - 1] + samples[mid]) / 2.0
        : samples[mid];

    double sq_diff_sum = 0;
    for (double v : samples)
        sq_diff_sum += (v - s.mean_ns) * (v - s.mean_ns);
    s.stddev_ns = samples.size() > 1
        ? std::sqrt(sq_diff_sum / static_cast<double>(samples.size() - 1))
        : 0.0;

    return s;
}

} // namespace detail

namespace {

namespace color {
    constexpr std::string_view reset = "\e[0m";
    constexpr std::string_view bold = "\e[1m";
    constexpr std::string_view dim = "\e[2m";
    constexpr std::string_view yellow = "\e[33m";
    constexpr std::string_view green = "\e[32m";
    constexpr std::string_view cyan = "\e[36m";
}

bool match_glob(std::string_view text, std::string_view pattern) {
    if (pattern.empty())
        return text.empty();
    std::size_t t = 0, p = 0;
    std::size_t star_p = std::string_view::npos, star_t = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++t;
            ++p;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star_p = p++;
            star_t = t;
        } else if (star_p != std::string_view::npos) {
            p = star_p + 1;
            t = ++star_t;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

bool matches_patterns(std::string_view nm, const std::vector<std::string>& patterns) {
    if (patterns.empty())
        return true;
    for (const auto& pat : patterns) {
        if (pat.find('*') != std::string::npos || pat.find('?') != std::string::npos) {
            if (match_glob(nm, pat))
                return true;
        } else if (nm.find(pat) != std::string_view::npos)
            return true;
    }
    return false;
}

std::string format_duration(double ns) {
    if (ns < 1000.0)
        return std::format("{:.3f} ns", ns);
    if (ns < 1000000.0)
        return std::format("{:.3f} us", ns / 1000.0);
    if (ns < 1000000000.0)
        return std::format("{:.3f} ms", ns / 1000000.0);
    return std::format("{:.3f} s", ns / 1000000000.0);
}

void print_entry(const entry_result& res, const options& opts) {
    if (opts.color)
        std::println("\n{}=== {} ==={}", color::bold, res.name, color::reset);
    else
        std::println("\n=== {} ===", res.name);

    for (const auto& sec : res.sections) {
        const auto& s = sec.st;
        const double rel_stddev = s.mean_ns > 0 ? (s.stddev_ns / s.mean_ns) * 100.0 : 0.0;
        const std::string range = std::format("[{} .. {}]", format_duration(s.min_ns), format_duration(s.max_ns));

        if (opts.color) {
            std::println("  {}{:<28}{}  {}{:>12}{}  {}{}{}  {}+-{:.1f}%{}  ({} iters)",
                    color::cyan, sec.name, color::reset, color::green,
                    format_duration(s.mean_ns), color::reset,
                    color::dim, range, color::reset, color::dim,
                    rel_stddev, color::reset, s.iterations);
        } else {
            std::println("  {:<28}  {:>12}  {}  +-{:.1f}%  ({} iters)",
                    sec.name, format_duration(s.mean_ns), range, rel_stddev, s.iterations);
        }
    }
}

} // namespace

int run_all(const options& opts) {
    auto& reg = detail::registry();

    if (opts.list_only) {
        std::println("Available benchmarks ({} total):", reg.size());
        for (const auto& e : reg) {
            std::string extra = e.is_skip ? " [skip]" : "";
            std::println("  {}{}", e.name, extra);
        }
        return 0;
    }

    std::vector<entry_result> results;
    std::size_t skipped = 0;

    for (const auto& e : reg) {
        if (!matches_patterns(e.name, opts.patterns))
            continue;

        if (e.is_skip) {
            ++skipped;
            if (opts.color)
                std::println("{}SKIP:{} {} ({})", color::yellow, color::reset, e.name, e.skip_reason);
            else
                std::println("SKIP: {} ({})", e.name, e.skip_reason);
            continue;
        }

        entry_result res;
        res.name = e.name;

        detail::run_config cfg{
            e.iterations_override ? e.iterations_override : opts.iterations,
            e.warmup_override ? e.warmup_override : opts.warmup_iterations,
        };
        detail::current_sections = &res.sections;
        detail::current_config = &cfg;

        struct Guard {
            ~Guard() {
                detail::current_sections = nullptr;
                detail::current_config = nullptr;
            }
        } guard;

        e.fn();

        if (res.sections.empty()) {
            std::println("{}: no BENCH sections were run (missing BENCH {{ ... }} in the function?)", e.name);
            continue;
        }

        print_entry(res, opts);
        results.push_back(std::move(res));
    }

    std::size_t total_sections = 0;
    for (const auto& r : results)
        total_sections += r.sections.size();

    if (opts.color)
        std::println("\n{}=== Benchmark Summary ==={}", color::bold, color::reset);
    else
        std::println("\n=== Benchmark Summary ===");
    std::println("# of benchmarks\t\t{}", results.size());
    std::println("# of sections\t\t{}", total_sections);
    if (skipped)
        std::println("# of skipped\t\t{}", skipped);

    return 0;
}

} // namespace dwhbll::bench
