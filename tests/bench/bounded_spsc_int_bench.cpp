#include <chrono>
#include <latch>
#include <optional>
#include <string>
#include <thread>

#include <dwhbll/concurrency/backoff/policy_exponential.h>
#include <dwhbll/concurrency/queues/bounded_spsc_queue.h>
#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/utils/threading.h>

// TODO: Make a benchmark harness and do this correctly!
bool bounded_spsc_int_bench(std::optional<std::string> _) {
    const std::size_t count = 2000000000;
    dwhbll::concurrency::queues::BoundedSPSCQueue<std::size_t, 8192, false, dwhbll::concurrency::backoff::PolicyExponential> channel;

    const auto threads = std::thread::hardware_concurrency();

    std::latch waiter(2);

    std::thread publisher([&] {
        std::size_t x = 1;

        if (threads > 2)
            dwhbll::utils::pin_thread_to_core(2);
        else if (threads == 2)
            dwhbll::utils::pin_thread_to_core(1);
        else
            dwhbll::utils::pin_thread_to_core(0);

        waiter.arrive_and_wait();

        while (x <= count) {
            channel.put(x++);
        }

        channel.put(0xFFFFFFFFFFFFFFFF);
    });

    dwhbll::utils::pin_thread_to_core(0);

    waiter.arrive_and_wait();

    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();

    std::size_t x = 0;
    std::size_t total = 0;

    while (true) {
        if (const auto next = channel.get(); next.has_value()) {
            if (next.value() == 0xFFFFFFFFFFFFFFFF)
                break;
            x++;
            total += next.value();
        }
    }

    auto now = std::chrono::steady_clock::now();

    if (total != count * (count + 1) / 2)
        dwhbll::debug::panic("[SPSC Queue] Total sum expected to be {}, got {}!", count * (count + 1) / 2, total);

    dwhbll::console::info("[SPSC Queue] Ran for {}, acquired {} messages, {} ops/msec",
      std::chrono::duration_cast<std::chrono::milliseconds>((now - start)),
      x,
      (double)x / (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count()
    );

    publisher.join();

    return false;
}
