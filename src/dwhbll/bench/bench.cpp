#include <dwhbll/bench/bench.h>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <numeric>
#include <cmath>
#include <print>
#include <string_view>

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <dwhbll/console/ansi_escape.h>
#include <dwhbll/debug/debug.h>
#include <dwhbll/stl_ext/string.h>

namespace dwhbll::bench {

namespace detail {

struct PerfGroup {
    struct Counter {
        uint64_t nr;
        uint64_t values[4];
    };

    int fds[4] = {-1, -1, -1, -1};
    bool available = false;

    // For whatever reason glibc does not have a wrapper for this
    static int perf_event_open(perf_event_attr* attr, int group_fd) {
        return static_cast<int>(syscall(SYS_perf_event_open, attr, 0, -1, group_fd, 0));
    }

    static int open_counter(uint32_t type, uint64_t config, int group_fd) {
        perf_event_attr attr{};
        attr.type = type;
        attr.size = sizeof(attr);
        attr.config = config;
        attr.disabled = 1;
        attr.exclude_kernel = 1;
        attr.exclude_hv = 1;
        attr.read_format = PERF_FORMAT_GROUP;
        return perf_event_open(&attr, group_fd);
    }

    // TODO: add more counters, and make this configurable
    // TODO: maybe use perf to time instead of clock difference when --perf is passed?
    PerfGroup() {
        fds[0] = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, -1);
        if (fds[0] < 0)
            return;
        fds[1] = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, fds[0]);
        fds[2] = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, fds[0]);
        fds[3] = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, fds[0]);
        if (fds[1] < 0 || fds[2] < 0 || fds[3] < 0) {
            close_all();
            return;
        }
        available = true;
    }

    ~PerfGroup() { close_all(); }

    PerfGroup(const PerfGroup&) = delete;
    PerfGroup& operator=(const PerfGroup&) = delete;

    void close_all() {
        for (int& fd : fds) {
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
        }
        available = false;
    }

    void reset_and_enable() {
        ioctl(fds[0], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        ioctl(fds[0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    }

    void disable() {
        ioctl(fds[0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    }

    std::array<uint64_t, 4> read_counts() {
        Counter buf{};
        if (read(fds[0], &buf, sizeof(buf)) < 0 || buf.nr < 4)
            return {};
        return {buf.values[0], buf.values[1], buf.values[2], buf.values[3]};
    }
};

static PerfGroup* current_perf = nullptr;

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

} // namespace detail

State::State(std::string_view name, std::source_location loc)
    : name_(name), loc_(loc) {
    const detail::run_config& cfg = detail::active_config();
    total_iterations_ = cfg.iterations;
    warmup_iterations_ = cfg.warmup_iterations;
}

void State::reset(std::string_view name, std::source_location loc, std::size_t total_iterations, std::size_t warmup_iterations) {
    name_ = name;
    loc_ = loc;
    state_ = state_kind::warmup;
    warmup_done_ = 0;
    measure_done_ = 0;
    total_iterations_ = total_iterations;
    warmup_iterations_ = warmup_iterations;
    paused_duration_ = std::chrono::nanoseconds{0};
    is_paused_ = false;
    samples_.clear();
    perf_samples_.clear();
    bytes_processed_ = 0;
    items_processed_ = 0;
}

void State::pause_timing() {
    if (!is_paused_) {
        if (detail::current_perf && detail::current_perf->available)
            detail::current_perf->disable();
        pause_start_ = std::chrono::steady_clock::now();
        is_paused_ = true;
    }
}

void State::resume_timing() {
    if (is_paused_) {
        paused_duration_ += std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - pause_start_);
        is_paused_ = false;
        if (detail::current_perf && detail::current_perf->available)
            detail::current_perf->reset_and_enable();
    }
}

bool State::next() {
    using clock = std::chrono::steady_clock;
    auto* perf = detail::current_perf;
    const bool use_perf = perf && perf->available;

    if (state_ == state_kind::warmup) [[unlikely]] {
        if (warmup_done_ < warmup_iterations_) {
            ++warmup_done_;
            return true;
        }
        state_ = state_kind::measure;
        samples_.reserve(total_iterations_);
        if (use_perf)
            perf_samples_.reserve(total_iterations_);
        paused_duration_ = std::chrono::nanoseconds{0};
        is_paused_ = false;
        if (use_perf)
            perf->reset_and_enable();
        start_ = clock::now();
        return true;
    }

    if (is_paused_)
        resume_timing();

    const auto now = clock::now();
    const double elapsed_ns = std::chrono::duration<double, std::nano>(now - start_
                                                        - paused_duration_).count();
    samples_.push_back(elapsed_ns);

    if (use_perf) {
        perf->disable();
        perf_samples_.push_back(perf->read_counts());
    }

    ++measure_done_;

    if (measure_done_ >= total_iterations_) {
        finalize();
        return false;
    }

    paused_duration_ = std::chrono::nanoseconds{0};
    is_paused_ = false;
    if (use_perf)
        perf->reset_and_enable();
    start_ = clock::now();
    return true;
}

void State::finalize() {
    ASSERT(detail::current_sections);

    section_result res;
    res.name = name_.empty() ? std::format("<line {}>", loc_.line()) : std::string(name_);
    res.loc = loc_;
    res.st = compute_stats(std::move(samples_));
    res.perf = compute_perf_stats(std::move(perf_samples_));
    res.bytes_processed = bytes_processed_;
    res.items_processed = items_processed_;

    std::lock_guard lock(detail::current_sections_mutex);
    detail::current_sections->push_back(std::move(res));
}

stats State::compute_stats(std::vector<double>&& samples) {
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

perf_stats State::compute_perf_stats(std::vector<std::array<uint64_t, 4>>&& samples) {
    perf_stats ps;
    if (samples.empty())
        return ps;
    ps.available = true;
    const double n = static_cast<double>(samples.size());
    for (const auto& s : samples) {
        ps.instructions += static_cast<double>(s[0]);
        ps.cycles += static_cast<double>(s[1]);
        ps.cache_misses += static_cast<double>(s[2]);
        ps.branch_misses += static_cast<double>(s[3]);
    }
    ps.instructions /= n;
    ps.cycles /= n;
    ps.cache_misses /= n;
    ps.branch_misses /= n;
    return ps;
}

namespace {

namespace color = console::ansi_escape::color;

std::string format_duration(double ns) {
    if (ns < 1000.0)
        return std::format("{:.3f} ns", ns);
    if (ns < 1000000.0)
        return std::format("{:.3f} us", ns / 1000.0);
    if (ns < 1000000000.0)
        return std::format("{:.3f} ms", ns / 1000000.0);
    return std::format("{:.3f} s", ns / 1000000000.0);
}

std::string format_bytes_per_sec(double bytes_per_sec) {
    if (bytes_per_sec < 1000.0)
        return std::format("{:.2f} B/s", bytes_per_sec);
    if (bytes_per_sec < 1000000.0)
        return std::format("{:.2f} KB/s", bytes_per_sec / 1000.0);
    if (bytes_per_sec < 1000000000.0)
        return std::format("{:.2f} MB/s", bytes_per_sec / 1000000.0);
    return std::format("{:.2f} GB/s", bytes_per_sec / 1000000000.0);
}

std::string format_items_per_sec(double items_per_sec) {
    if (items_per_sec < 1000.0)
        return std::format("{:.2f} items/s", items_per_sec);
    if (items_per_sec < 1000000.0)
        return std::format("{:.2f} Kitems/s", items_per_sec / 1000.0);
    if (items_per_sec < 1000000000.0)
        return std::format("{:.2f} Mitems/s", items_per_sec / 1000000.0);
    return std::format("{:.2f} Gitems/s", items_per_sec / 1000000000.0);
}

void print_entry(const entry_result& res, const options& opts, bool is_first) {
    std::string_view prefix = is_first ? "" : "\n";
    if (opts.color)
        std::println("{}{}=== {} ==={}", prefix, color::bold, res.name, color::reset);
    else
        std::println("{}=== {} ===", prefix, res.name);

    for (const auto& sec : res.sections) {
        const auto& s = sec.st;
        const double rel_stddev = s.mean_ns > 0 ? (s.stddev_ns / s.mean_ns) * 100.0 : 0.0;

        if (res.sections.size() > 1)
            std::println("  {}:", sec.name);

        auto row = [&](std::string_view label, std::string value) {
            std::println("    {:<18} {}", label, value);
        };
        auto row_colored = [&](std::string_view label, std::string value) {
            if (opts.color)
                std::println("    {:<18} {}{}{}", label, color::green, value, color::reset);
            else
                std::println("    {:<18} {}", label, value);
        };

        row_colored("mean:", format_duration(s.mean_ns));
        row("median:", format_duration(s.median_ns));
        row("min:", format_duration(s.min_ns));
        row("max:", format_duration(s.max_ns));
        row("stddev:", std::format("{} (+-{:.1f}%)", format_duration(s.stddev_ns), rel_stddev));
        row("iterations:", std::format("{}", s.iterations));

        if (sec.bytes_processed > 0 && s.mean_ns > 0) {
            const double rate = static_cast<double>(sec.bytes_processed) / (s.mean_ns / 1e9);
            row("throughput:", format_bytes_per_sec(rate));
        } else if (sec.items_processed > 0 && s.mean_ns > 0) {
            const double rate = static_cast<double>(sec.items_processed) / (s.mean_ns / 1e9);
            row("throughput:", format_items_per_sec(rate));
        }

        if (opts.perf && sec.perf.available) {
            const double ipc = sec.perf.cycles > 0 ? sec.perf.instructions / sec.perf.cycles : 0.0;
            row("instructions:", std::format("{:.0f}", sec.perf.instructions));
            row("cycles:", std::format("{:.0f}", sec.perf.cycles));
            row("IPC:", std::format("{:.3f}", ipc));
            row("cache misses:", std::format("{:.0f}", sec.perf.cache_misses));
            row("branch misses:", std::format("{:.0f}", sec.perf.branch_misses));
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

    std::unique_ptr<detail::PerfGroup> perf_group;
    if (opts.perf) {
        perf_group = std::make_unique<detail::PerfGroup>();
        if (!perf_group->available)
            std::println("warning: perf_event_open failed; perf counters unavailable");
        detail::current_perf = perf_group.get();
    }

    struct PerfGuard {
        ~PerfGuard() { detail::current_perf = nullptr; }
    } perf_guard;

    std::vector<entry_result> results;
    std::size_t skipped = 0;
    bool is_first = true;

    for (const auto& e : reg) {
        if (!stl_ext::matches_patterns(e.name, opts.patterns))
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

        state.reset(e.name, std::source_location::current(), cfg.iterations, cfg.warmup_iterations);

        e.fn();

        if (res.sections.empty()) {
            std::println("{}: no BENCH sections were run", e.name);
            continue;
        }

        print_entry(res, opts, is_first);
        is_first = false;
        results.push_back(std::move(res));
    }

    std::size_t total_sections = 0;
    for (const auto& r : results)
        total_sections += r.sections.size();

    if (opts.color)
        std::println("\n{}=== Benchmark Summary ==={}", color::bold, color::reset);
    else
        std::println("\n=== Benchmark Summary ===");
    std::println("  {:<26} {}", "# of benchmarks", results.size());
    std::println("  {:<26} {}", "# of sections", total_sections);
    if (skipped)
        std::println("  {:<26} {}", "# of skipped", skipped);

    return 0;
}

} // namespace dwhbll::bench
