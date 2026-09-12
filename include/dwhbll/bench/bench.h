#pragma once

#include <dwhbll/meta/meta.h>

#include <meta>
#include <chrono>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace dwhbll::bench {

struct bench_marker {};
inline constexpr bench_marker bench{};

struct name {
    char const* bench_name;

    consteval explicit name(std::string_view name_ = "")
        : bench_name(std::define_static_string(name_)) {}
};

struct skip {
    char const* reason;

    consteval explicit skip(std::string_view reason_ = "")
        : reason(std::define_static_string(reason_)) {}
};

struct iterations {
    std::size_t count;

    consteval explicit iterations(std::size_t count_ = 1000)
        : count(count_) {}
};

struct warmup {
    std::size_t count;

    consteval explicit warmup(std::size_t count_ = 3)
        : count(count_) {}
};

struct stats {
    double min_ns = 0;
    double max_ns = 0;
    double mean_ns = 0;
    double median_ns = 0;
    double stddev_ns = 0;
    std::size_t iterations = 0;
};

struct section_result {
    std::string name;
    std::source_location loc;
    stats st;
};

struct entry_result {
    std::string name;
    std::vector<section_result> sections;
};

struct options {
    std::vector<std::string> patterns;
    bool list_only = false;
    bool color = true;

    std::size_t warmup_iterations = 3;
    std::size_t iterations = 1000;
};

namespace detail {

struct entry {
    std::string name;
    void (*fn)();
    bool is_skip;
    std::string_view skip_reason;
    // 0 is harness options
    std::size_t iterations_override = 0;
    std::size_t warmup_override = 0;
};

std::vector<entry>& registry();

class section {
public:
    section(std::string_view name, std::source_location loc);

    bool next();

private:
    void finalize();

    static stats compute_stats(std::vector<double>&& samples);

    enum class state { warmup, measure };

    std::string_view name_;
    std::source_location loc_;
    state state_ = state::warmup;
    std::size_t warmup_done_ = 0;
    std::size_t measure_done_ = 0;
    std::size_t total_iterations_ = 0;
    std::size_t warmup_iterations_ = 0;
    std::chrono::steady_clock::time_point start_;
    std::vector<double> samples_;
};

struct discovery_traits {
    using marker_type = bench_marker;

    template <std::meta::info func, std::meta::info scope>
    static constexpr void process() {
        using namespace std::meta;
        if constexpr (annotations_of_with_type(func, ^^bench_marker).empty())
            return;

        static_assert(!is_class_member(func) || is_static_member(func),
                      "bench annotation on non-static member functions is not allowed.");

        constexpr auto fn = extract<void(*)()>(func);
        auto& reg = registry();

        for (const auto& e : reg) {
            if (e.fn == fn)
                return;
        }

        constexpr auto name_ann = dwhbll::meta::find_annotation(func, ^^name);
        std::string bench_name;
        if constexpr (name_ann != info()) {
            static constexpr auto name_val =
                extract<typename[: type_of(name_ann) :]>(name_ann);
            bench_name = std::string_view(name_val.bench_name);
        } else {
            static_assert(has_identifier(func),
                "bench with no name given on a function with no identifier");
            bench_name = identifier_of(func);
        }
        if constexpr (has_identifier(scope) && identifier_of(scope) != "::")
            bench_name = std::string(identifier_of(scope)) + "/" + bench_name;

        constexpr auto skip_ann = dwhbll::meta::find_annotation(func, ^^skip);
        constexpr bool is_skip = skip_ann != info();
        std::string_view skip_reason;
        if constexpr (is_skip) {
            static constexpr auto skip_val =
                extract<typename[: type_of(skip_ann) :]>(skip_ann);
            skip_reason = std::string_view(skip_val.reason);
        }

        constexpr auto iters_ann = dwhbll::meta::find_annotation(func, ^^iterations);
        std::size_t iters_override = 0;
        if constexpr (iters_ann != info()) {
            static constexpr auto iters_val =
                extract<typename[: type_of(iters_ann) :]>(iters_ann);
            iters_override = iters_val.count;
        }

        constexpr auto warmup_ann = dwhbll::meta::find_annotation(func, ^^warmup);
        std::size_t warmup_override = 0;
        if constexpr (warmup_ann != info()) {
            static constexpr auto warmup_val =
                extract<typename[: type_of(warmup_ann) :]>(warmup_ann);
            warmup_override = warmup_val.count;
        }

        reg.push_back({ bench_name, fn, is_skip, skip_reason, iters_override, warmup_override });
    }
};

} // namespace detail

int run_all(const options& opts = {});

} // namespace dwhbll::bench

#define BENCH \
    for (::dwhbll::bench::detail::section _dwhbll_bench_section{ "",\
                std::source_location::current() }; _dwhbll_bench_section.next(); )

#define BENCH_NAMED(section_name) \
    for (::dwhbll::bench::detail::section _dwhbll_bench_section{ (section_name),\
            std::source_location::current() }; _dwhbll_bench_section.next(); )

#define BENCH_REGISTER_FILE() \
    namespace { static const bool _dwhbll_bench_registered = \
        (::dwhbll::meta::collect_annotated<::dwhbll::bench::detail::discovery_traits, ^^::, \
            ::dwhbll::meta::fixed_string(__FILE__)>(), true); }
