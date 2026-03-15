#include <chrono>
#include <optional>
#include <string>
#include <thread>

#include <dwhbll/concurrency/queues/bounded_spsc_queue.h>
#include <dwhbll/console/Logging.h>

// TODO: Make a benchmark harness and do this correctly!
bool bounded_spsc_int_bench(std::optional<std::string> _) {
    dwhbll::concurrency::queues::BoundedSPSCQueue<std::size_t, 8192> channel;

    std::thread publisher([&] {
        std::size_t x = 0;

        while (x < 500000000) {
            if (channel.put(x))
                x++;
        }

        channel.put(0xFFFFFFFFFFFFFFFF);
    });

    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();

    std::size_t x = 0;

    while (true) {
        if (const auto next = channel.get(); next.has_value()) {
          if (next.value() == 0xFFFFFFFFFFFFFFFF)
            break;
          x++;
        }
    }

    auto now = std::chrono::steady_clock::now();

    dwhbll::console::info("[SPSC Queue] Ran for {}, acquired {} messages, {} ops/msec",
      std::chrono::duration_cast<std::chrono::milliseconds>((now - start)),
      x,
      (double)x / (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count()
    );

    publisher.join();

    return false;
}
