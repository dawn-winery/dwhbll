#include <chrono>
#include <latch>
#include <optional>
#include <string>
#include <thread>

#include <dwhbll/concurrency/recycling_concurrent_stack.h>
#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/utils/threading.h>

// TODO: Make a benchmark harness and do this correctly!
bool recycling_concurrent_stack_bench(std::optional<std::string> _) {
    constexpr std::size_t items = 500000000;

    dwhbll::concurrency::RecyclingConcurrentStack<int> stack;

    const auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;

    const auto available_cores = std::thread::hardware_concurrency();
    const auto count = std::min(available_cores / 2, 1u);

    std::latch waiter(count * 2);

    for (int i = 0; i < count; i++) {
        threads.emplace_back([&] {
            std::size_t x = 1;

            // take evens
            dwhbll::utils::pin_thread_to_core(i);

            waiter.arrive_and_wait();

            while (x <= items)
                stack.push(x++);
        });
    }

    std::vector<std::size_t> totals(count);

    for (int i = 0; i < count; i++) {
        threads.emplace_back([=, &stack, &totals, &waiter] {
            std::size_t x = 0;
            std::size_t total = 0;

            // take odds
            dwhbll::utils::pin_thread_to_core(available_cores == 1 ? 0 : i + count);

            waiter.arrive_and_wait();

            while (x < items) {
                if (const auto& v = stack.pop(); v.has_value()) {
                    total += v.value();
                    x++;
                }
            }

            totals[i] = total;
        });
    }

    for (auto& thread : threads)
        thread.join();

    const auto now = std::chrono::steady_clock::now();

    const std::size_t sum = items*(items+1)/2*count;
    std::size_t total = 0;

    for (const auto x : totals)
        total += x;

    dwhbll::debug::cond_assert(sum == total, "expected sum {}, got {}", sum, total);

    dwhbll::console::info("[Recycling Concurrent Stack] Ran for {}, push/pop total {} ops, {} ops/msec",
      std::chrono::duration_cast<std::chrono::milliseconds>((now - start)),
      items * count,
      static_cast<double>(items * count) / static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count())
    );

    return false;
}