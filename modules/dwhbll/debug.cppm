module;

#include <dwhbll/debug/panic.h>
#include <dwhbll/debug/debug.h>
#include <dwhbll/debug/format.h>
#include <dwhbll/debug/perf.h>

export module dwhbll.debug;

export namespace dwhbll::debug {
    using dwhbll::debug::panic;

    using dwhbll::debug::assert_internal;
    using dwhbll::debug::cond_assert;
    using dwhbll::debug::is_being_debugged;
    using dwhbll::debug::unreachable;
    using dwhbll::debug::todo;
    using dwhbll::debug::demangle;

    using dwhbll::debug::set_perf_level;
    using dwhbll::debug::time;

    using dwhbll::debug::task_deferral;
    using dwhbll::debug::running_tasks;
}

#if __cpp_impl_reflection >= 202506L
export namespace dwhbll::meta {
    using dwhbll::meta::dbg;
}
#endif
