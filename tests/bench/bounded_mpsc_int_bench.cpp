#include <chrono>
#include <optional>
#include <string>
#include <thread>

#include <dwhbll/concurrency/queues/bounded_mpsc_queue.h>
#include <dwhbll/console/Logging.h>

// TODO: Make a benchmark harness and do this correctly!
bool bounded_mpsc_int_bench(std::optional<std::string> _) {
    dwhbll::concurrency::queues::BoundedMPSCQueue<std::size_t, 8192> channel;

    std::vector<std::thread> threads;

    for (int i = 0; i < 8; i++) {
        threads.emplace_back([&] {
            std::size_t x = 0;

            while (x < 50000000) {
                if (channel.put(x))
                    x++;
            }

            channel.put(0xFFFFFFFFFFFFFFFF);
        });
    }

    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();

    std::size_t x = 0;

    std::size_t done = 0;

    while (true) {
        if (const auto next = channel.get(); next.has_value()) {
            if (++x == 50000000 * 8)
            // if (next.value() == 0xFFFFFFFFFFFFFFFF && ++done == 8)
                break;
        }
    }

    auto now = std::chrono::steady_clock::now();

    dwhbll::console::info("[MPSC Queue] Ran for {}, acquired {} messages, {} ops/msec",
      std::chrono::duration_cast<std::chrono::milliseconds>((now - start)),
      x,
      (double)x / (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count()
    );

    for (auto& thread : threads)
        thread.join();

    return false;
}
