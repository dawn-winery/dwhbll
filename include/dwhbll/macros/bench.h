#pragma once

#define BENCH \
    while (::dwhbll::bench::state.keep_running())

#define BENCH_REGISTER_FILE() \
    namespace { static const bool _dwhbll_bench_registered = \
        (::dwhbll::meta::collect_annotated<::dwhbll::bench::detail::discovery_traits, ^^::, \
            ::dwhbll::meta::fixed_string(__FILE__)>(), true); }
