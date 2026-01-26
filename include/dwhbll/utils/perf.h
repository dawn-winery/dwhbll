#pragma once

#include <chrono>
#include <string>
#include <dwhbll/console/Logging.h>

namespace dwhbll::utils {
    void set_perf_level(console::Level level);

    struct time {
    private:
        std::string stage_name;
        std::chrono::steady_clock::time_point start;

    public:
        explicit time(const std::string& stage);

        ~time();
    };
}

/**
 * @brief use this in a scope to time the duration of the scope.
 * @param stage The stage string that will be output, this just needs to be convertible to std::string.
 *
 * Example usage:
 * ```c++
 * {
 *   timeit("test function");
 *
 *   // slow running thing for demo purposes
 *   using namespace std::chrono_literals;
 *   std::this_thread::sleep_for(5s);
 * }
 * ```
 */
#define timeit(stage) ::dwhbll::utils::time __dwhbll_scope_timer__(stage)
