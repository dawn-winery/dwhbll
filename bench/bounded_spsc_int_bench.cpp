#include <chrono>
#include <latch>
#include <optional>
#include <string>
#include <thread>

#include <dwhbll/bench/bench.h>
#include <dwhbll/concurrency/backoff/policy_exponential.h>
#include <dwhbll/concurrency/queues/bounded_spsc_queue.h>
#include <dwhbll/concurrency/threading.h>
#include <dwhbll/console/logging.h>
#include <dwhbll/debug/debug.h>

[[=dwhbll::bench::bench]]
[[=dwhbll::bench::iterations(5)]]
[[=dwhbll::bench::warmup(2)]]
void bounded_spsc_int_bench() {
    constexpr std::size_t count = 5000000;
    constexpr std::size_t total_iterations = 7;

    dwhbll::concurrency::queues::BoundedSPSCQueue<std::size_t, 8192, false, dwhbll::concurrency::backoff::PolicyExponential> channel;

    const auto threads = std::thread::hardware_concurrency();

    std::latch waiter(2);

    std::thread publisher([&] {
        std::size_t x = 1;

        if (threads > 2)
            dwhbll::concurrency::pin_thread_to_core(2);
        else if (threads == 2)
            dwhbll::concurrency::pin_thread_to_core(1);
        else
            dwhbll::concurrency::pin_thread_to_core(0);

        waiter.arrive_and_wait();

        const std::size_t max_x = count * total_iterations;
        while (x <= max_x) {
            channel.put(x++);
        }

        channel.put(0xFFFFFFFFFFFFFFFF);
    });

    dwhbll::concurrency::pin_thread_to_core(0);

    waiter.arrive_and_wait();

    std::size_t total = 0;

    BENCH {
        std::size_t x = 0;
        while (x < count) {
            if (const auto next = channel.get(); next.has_value()) {
                if (next.value() == 0xFFFFFFFFFFFFFFFF)
                    break;
                x++;
                total += next.value();
            }
        }
    }

    publisher.join();

    const std::size_t expected = (count * total_iterations) * (count * total_iterations + 1) / 2;
    if (total != expected)
        dwhbll::debug::panic("[SPSC Queue] Total sum expected to be {}, got {}!", expected, total);
}

BENCH_REGISTER_FILE()
