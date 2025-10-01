#pragma once

#include <version>

#ifndef __cpp_lib_stacktrace
#include <dwhbll/utils/stacktrace.hpp>
#else
#include <stacktrace>
#endif

#include <stdexcept>

namespace dwhbll::exceptions {
    class rt_exception_base : public std::runtime_error {
#ifndef __cpp_lib_stacktrace
        dwhbll::stacktrace::trace trace;
#else
        std::stacktrace trace;
#endif

        void populate_trace();

    public:
        rt_exception_base() : std::runtime_error("") {
            populate_trace();
        }

        rt_exception_base(const rt_exception_base &other) : std::runtime_error(other) {
            trace = other.trace;
        }

        rt_exception_base(const std::string& what_arg) : std::runtime_error(what_arg) {
            populate_trace();
        }

        rt_exception_base(const char* what_arg) : std::runtime_error(what_arg) {
            populate_trace();
        }

        [[nodiscard]] std::string get_prettyprint_trace() const;

        void trace_to_stderr() const;
    };
}
