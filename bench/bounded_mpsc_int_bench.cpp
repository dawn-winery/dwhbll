#include <chrono>
#include <latch>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <dwhbll/bench/bench.h>
#include <dwhbll/concurrency/backoff/policy_exponential.h>
#include <dwhbll/concurrency/queues/bounded_mpsc_queue.h>
#include <dwhbll/concurrency/threading.h>
#include <dwhbll/console/logging.h>
#include <dwhbll/debug/debug.h>

[[=dwhbll::bench::bench]]
[[=dwhbll::bench::iterations(5)]]
[[=dwhbll::bench::warmup(2)]]
void bounded_mpsc_int_bench() {
    constexpr std::size_t count = 5000000;
    constexpr std::size_t total_iterations = 7;

    dwhbll::concurrency::queues::BoundedMPSCQueue<std::size_t, 8192, false, dwhbll::concurrency::backoff::PolicyExponential> channel;

    const auto thread_count = std::thread::hardware_concurrency();

    const auto producer_count = std::max(thread_count / 2 - 1, 1u);

    std::latch waiter(producer_count + 1);

    std::vector<std::thread> threads;

    for (uint32_t i = 0; i < producer_count; i++) {
        threads.emplace_back([&] {
            std::size_t x = 1;
            const std::size_t max_x = count * total_iterations;

            // dwhbll::concurrency::pin_thread_to_core(i * 2);

            waiter.arrive_and_wait();

            while (x <= max_x) {
                if (channel.put(x))
                    x++;
            }
        });
    }

    // dwhbll::concurrency::pin_thread_to_core(producer_count);

    waiter.arrive_and_wait();

    std::size_t total = 0;
    const std::size_t target = count * producer_count;

    BENCH {
        std::size_t x = 0;
        while (true) {
            if (const auto next = channel.get(); next.has_value()) {
                total += next.value();
                if (++x == target)
                    break;
            }
        }
    }

    const std::size_t expected = ((count * total_iterations) * (count * total_iterations + 1) / 2) * producer_count;

    if (total != expected)
        dwhbll::debug::panic("[MPSC Queue] Incorrect queue behavior! Total sum of all elements {} instead of {}", total, expected);

    for (auto& thread : threads)
        thread.join();
}

BENCH_REGISTER_FILE()
