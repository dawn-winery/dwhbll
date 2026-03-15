#pragma once

#include <atomic>
#include <optional>
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace dwhbll::concurrency::queues {

#if __cpp_lib_hardware_interference_size >= 201703L
    constexpr std::size_t AlignmentSize = std::hardware_destructive_interference_size;
#else
    constexpr std::size_t AlignmentSize = 64;
#endif

    template<typename T, std::size_t N, bool FailOnFull = false>
    class BoundedSPSCQueue {
        static_assert(__builtin_popcountll(N) == 1, "BoundedSPSCQueue expects N to be a power of 2!");
        static_assert(N > 1, "BoundedSPSCQueue has to be bigger than 1! (One entry is used to know if queue is full)");

        static constexpr std::size_t MASK = N - 1;

        alignas(AlignmentSize) std::atomic_uint64_t head;
        alignas(AlignmentSize) std::atomic_uint64_t tail;

        T buffer[N];

    public:
        BoundedSPSCQueue() {
            head.store(0, std::memory_order_relaxed);
            tail.store(0, std::memory_order_relaxed);
        }

        bool put(T value) {
            auto t = tail.load(std::memory_order_relaxed);
            const auto next = (t + 1) & MASK;

            if constexpr (FailOnFull) {
                if (next == head.load(std::memory_order_acquire))
                    return false;
            } else {
                while (next == head.load(std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(__i386__)
                    _mm_pause();
#endif
                }
            }

            buffer[t] = value;

            tail.store(next, std::memory_order_release);

            return true;
        }

        std::optional<T> get() {
            auto h = head.load(std::memory_order_relaxed);

            if (h == tail.load(std::memory_order_acquire))
                return std::nullopt; // nothing in the queue currently

            auto result = buffer[h];

            head.store((h + 1) & MASK, std::memory_order_release);

            return result;
        }

        T getBlocking() {
            auto h = head.load(std::memory_order_relaxed);

            while (h == tail.load(std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(__i386__)
                _mm_pause();
#endif
            }

            auto result = buffer[h];

            head.store((h + 1) & MASK, std::memory_order_release);

            return result;
        }
    };
}
