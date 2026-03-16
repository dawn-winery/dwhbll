#pragma once

#include <atomic>
#include <optional>
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

#include <dwhbll/concurrency/common.h>

namespace dwhbll::concurrency::queues {
    template<typename T, std::size_t N, bool FailOnFull = false>
    class BoundedMPSCQueue {
        static_assert(__builtin_popcountll(N) == 1, "BoundedSPSCQueue expects N to be a power of 2!");
        static_assert(N > 1, "BoundedSPSCQueue has to be bigger than 1! (One entry is used to know if queue is full)");

        static constexpr std::size_t MASK = N - 1;

        struct Cell {
            std::atomic<std::size_t> seq;
            T data;
        };

        alignas(AlignmentSize) std::size_t head;
        alignas(AlignmentSize) std::atomic_size_t tail;

        Cell buffer[N];

    public:
        BoundedMPSCQueue() {
            head = 0;
            tail.store(0, std::memory_order_relaxed);

            for (int i = 0; i < N; i++)
                buffer[i].seq.store(i, std::memory_order_relaxed);

            std::atomic_thread_fence(std::memory_order_release);
        }

        template <typename... Args>
        bool put(Args&&... args) {
            const std::size_t pos = tail.fetch_add(1, std::memory_order_relaxed);

            Cell* loc = &buffer[pos & MASK];

            while (true) {
                const std::size_t seq = loc->seq.load(std::memory_order_acquire);

                const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

                if (diff == 0)
                    break;

                if constexpr (FailOnFull)
                    if (diff < 0)
                        return false; // queue full

#if defined(__x86_64__) || defined(__i386__)
                _mm_pause();
#endif
            }

            new (&loc->data) T(std::forward<Args>(args)...);

            loc->seq.store(pos + 1, std::memory_order_release);

            return true;
        }

        std::optional<T> get() {
            Cell* c = &buffer[head & MASK];

            const std::size_t seq = c->seq.load(std::memory_order_acquire);

            if (const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(head + 1); diff < 0)
                return std::nullopt; // nothing in the queue currently

            auto result = std::move(c->data);
            c->data.~T();

            c->seq.store(head + N, std::memory_order_release);

            head++;

            return result;
        }

        T getBlocking() {
            Cell* c = &buffer[head & MASK];

            while (true) {
                const std::size_t seq = c->seq.load(std::memory_order_acquire);

                if (const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(head + 1); diff >= 0)
                    break;

#if defined(__x86_64__) || defined(__i386__)
                _mm_pause();
#endif
            }

            auto result = std::move(c->data);
            c->data.~T();

            c->seq.store(head + N, std::memory_order_release);

            head++;

            return result;
        }
    };
}
