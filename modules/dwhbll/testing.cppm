module;

#include <dwhbll/testing/testing.h>
#include <dwhbll/testing/harness.h>

export module dwhbll.testing;

export import dwhbll.meta;

export namespace dwhbll::test {
    using dwhbll::test::test_marker;
    using dwhbll::test::test;

    using dwhbll::test::name;
    using dwhbll::test::skip;
    using dwhbll::test::xfail;

    using dwhbll::test::expect;
    using dwhbll::test::expect_true;
    using dwhbll::test::expect_false;
    using dwhbll::test::expect_null;
    using dwhbll::test::expect_not_null;
    using dwhbll::test::expect_eq;
    using dwhbll::test::expect_ne;
    using dwhbll::test::expect_lt;
    using dwhbll::test::expect_le;
    using dwhbll::test::expect_gt;
    using dwhbll::test::expect_ge;
    using dwhbll::test::expect_throws;
    using dwhbll::test::expect_no_throw;

    using dwhbll::test::failure;
    using dwhbll::test::test_status;
    using dwhbll::test::to_status_string;
    using dwhbll::test::test_result;
    using dwhbll::test::test_info;
    using dwhbll::test::test_start_callback;
    using dwhbll::test::test_end_callback;
    using dwhbll::test::options;
    using dwhbll::test::summary_counts;
    using dwhbll::test::suite_result;
    using dwhbll::test::test_harness;
    using dwhbll::test::default_harness;
    using dwhbll::test::runner;

    using dwhbll::test::run_all;

    namespace detail {
        using dwhbll::test::detail::result;
        using dwhbll::test::detail::current_result;
        using dwhbll::test::detail::report_failure;
        using dwhbll::test::detail::entry;
        using dwhbll::test::detail::registry;
        using dwhbll::test::detail::discovery_traits;
    }
}
