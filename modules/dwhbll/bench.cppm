module;

#include <dwhbll/bench/bench.h>

export module dwhbll.bench;

export namespace dwhbll::bench {
    using dwhbll::bench::bench_marker;
    using dwhbll::bench::bench;
    using dwhbll::bench::name;
    using dwhbll::bench::skip;
    using dwhbll::bench::iterations;
    using dwhbll::bench::warmup;
    using dwhbll::bench::stats;
    using dwhbll::bench::perf_stats;
    using dwhbll::bench::section_result;
    using dwhbll::bench::entry_result;
    using dwhbll::bench::options;
    using dwhbll::bench::State;
    using dwhbll::bench::state;
    using dwhbll::bench::run_all;

    namespace detail {
        using dwhbll::bench::detail::entry;
        using dwhbll::bench::detail::registry;
        using dwhbll::bench::detail::discovery_traits;
    }
}
