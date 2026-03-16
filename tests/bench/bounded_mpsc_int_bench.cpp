#include <chrono>
#include <latch>
#include <optional>
#include <string>
#include <thread>

#include <dwhbll/concurrency/backoff/policy_exponential.h>
#include <dwhbll/concurrency/queues/bounded_mpsc_queue.h>
#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/utils/threading.h>

// TODO: Make a benchmark harness and do this correctly!
bool bounded_mpsc_int_bench(std::optional<std::string> _) {
    constexpr std::size_t count = 50000000;

    dwhbll::concurrency::queues::BoundedMPSCQueue<std::size_t, 8192, false, dwhbll::concurrency::backoff::PolicyExponential> channel;

    const auto thread_count = std::thread::hardware_concurrency();

    const auto producer_count = std::max(thread_count / 2 - 1, 1u);

    std::latch waiter(producer_count + 1);

    std::vector<std::thread> threads;

    for (int i = 0; i < producer_count; i++) {
        threads.emplace_back([&] {
            std::size_t x = 1;

            // dwhbll::utils::pin_thread_to_core(i * 2);

            waiter.arrive_and_wait();

            while (x <= count) {
                if (channel.put(x))
                    x++;
            }
        });
    }

    // dwhbll::utils::pin_thread_to_core(producer_count);

    waiter.arrive_and_wait();

    using namespace std::chrono_literals;
    const auto start = std::chrono::steady_clock::now();

    std::size_t total = 0;

    std::size_t x = 0;

    while (true) {
        if (const auto next = channel.get(); next.has_value()) {
            total += next.value();
            if (++x == count * producer_count)
                break;
        }
    }

    const auto now = std::chrono::steady_clock::now();

    if (total != count * (count + 1) / 2 * producer_count)
        dwhbll::debug::panic("[MPSC Queue] Incorrect queue behavior! Total sum of all elements {} instead of {}", total, count * (count + 1) / 2 * producer_count);

    dwhbll::console::info("[MPSC Queue] Ran for {}, acquired {} messages, {} ops/msec",
      std::chrono::duration_cast<std::chrono::milliseconds>((now - start)),
      x,
      static_cast<double>(x) / static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count())
    );

    for (auto& thread : threads)
        thread.join();

    return false;
}
