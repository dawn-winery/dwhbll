#include <chrono>
#include <latch>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <dwhbll/bench/bench.h>
#include <dwhbll/concurrency/recycling_concurrent_stack.h>
#include <dwhbll/concurrency/threading.h>
#include <dwhbll/console/logging.h>
#include <dwhbll/debug/debug.h>

[[=dwhbll::bench::bench]]
[[=dwhbll::bench::iterations(5)]]
[[=dwhbll::bench::warmup(2)]]
void recycling_concurrent_stack_bench() {
    constexpr std::size_t items = 2000000;
    constexpr std::size_t total_iterations = 7;

    dwhbll::concurrency::RecyclingConcurrentStack<int> stack;

    std::vector<std::thread> threads;

    const auto available_cores = std::thread::hardware_concurrency();
    const auto count = std::min(available_cores / 2, 1u);

    std::vector<std::latch*> start_latches;
    std::vector<std::latch*> done_latches;

    for (std::size_t i = 0; i < total_iterations; ++i) {
        start_latches.push_back(new std::latch(count * 2 + 1));
        done_latches.push_back(new std::latch(count * 2 + 1));
    }

    for (uint32_t i = 0; i < count; i++) {
        threads.emplace_back([&, i] {
            // take evens
            dwhbll::concurrency::pin_thread_to_core(i);

            for (std::size_t iter = 0; iter < total_iterations; ++iter) {
                start_latches[iter]->arrive_and_wait();
                std::size_t x = 1;
                while (x <= items)
                    stack.push(x++);
                done_latches[iter]->arrive_and_wait();
            }
        });
    }

    std::vector<std::size_t> totals(count, 0);

    for (uint32_t i = 0; i < count; i++) {
        threads.emplace_back([=, &stack, &totals, &start_latches, &done_latches] {
            // take odds
            dwhbll::concurrency::pin_thread_to_core(available_cores == 1 ? 0 : i + count);

            for (std::size_t iter = 0; iter < total_iterations; ++iter) {
                start_latches[iter]->arrive_and_wait();
                std::size_t x = 0;
                std::size_t total = 0;
                while (x < items) {
                    if (const auto& v = stack.pop(); v.has_value()) {
                        total += v.value();
                        x++;
                    }
                }
                totals[i] += total;
                done_latches[iter]->arrive_and_wait();
            }
        });
    }

    std::size_t iter_idx = 0;

    BENCH {
        start_latches[iter_idx]->arrive_and_wait();
        done_latches[iter_idx]->arrive_and_wait();
        iter_idx++;
    }

    for (auto& thread : threads)
        thread.join();

    for (auto* l : start_latches)
        delete l;
    for (auto* l : done_latches)
        delete l;

    const std::size_t sum = (items * (items + 1) / 2 * count) * total_iterations;
    std::size_t total = 0;

    for (const auto x : totals)
        total += x;

    dwhbll::debug::cond_assert(sum == total, "expected sum {}, got {}", sum, total);
}

BENCH_REGISTER_FILE()
