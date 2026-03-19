#pragma once

#include <atomic>
#include <optional>
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

#include <dwhbll/concurrency/backoff/policy_pause.h>
#include <dwhbll/concurrency/backoff/backoff_policy.h>
#include <dwhbll/concurrency/common.h>

namespace dwhbll::concurrency::queues {

    template<typename T, std::size_t N, bool FailOnFull = false, typename BackoffPolicy = backoff::PolicyPause>
    requires backoff::BackoffPolicy<BackoffPolicy>
    class BoundedSPSCQueue {
        static_assert(__builtin_popcountll(N) == 1, "BoundedSPSCQueue expects N to be a power of 2!");
        static_assert(N > 1, "BoundedSPSCQueue has to be bigger than 1! (One entry is used to know if queue is full)");

        static constexpr std::size_t MASK = N - 1;

        alignas(AlignmentSize) std::atomic_uint64_t head;
        size_t cached_tail = 0;

        alignas(AlignmentSize) std::atomic_uint64_t tail;
        size_t cached_head = 0;

        T buffer[N];

    public:
        BoundedSPSCQueue() {
            head.store(0, std::memory_order_relaxed);
            tail.store(0, std::memory_order_relaxed);
        }

        template <typename... Args>
        bool put(Args&&... args) {
            auto t = tail.load(std::memory_order_relaxed);
            const auto next = (t + 1) & MASK;


            if (cached_head == next) {	// Local cache thinks the queue is full

            	// We load the atomic to check
            	cached_head = head.load(std::memory_order_acquire);

	            if constexpr (FailOnFull) {
	                if (next == cached_head)
	                    return false;
	            } else {
	                BackoffPolicy policy;
	                while (next == cached_head) {
	                    policy.pause();
						cached_head = head.load(std::memory_order_acquire);
	                }
	            }
        	}

            new(buffer + t) T(std::forward<Args...>(args...));

            tail.store(next, std::memory_order_release);

            return true;
        }

        std::optional<T> get() {
            auto h = head.load(std::memory_order_relaxed);

            if (h == cached_tail) {	 // queue empty maybe
           		cached_tail = tail.load(std::memory_order_acquire);
           		if (h == cached_tail)
                	return std::nullopt; // nothing in the queue currently

            }

            auto result = std::move(buffer[h]);
            buffer[h].~T();

            head.store((h + 1) & MASK, std::memory_order_release);

            return result;
        }

        T getBlocking() {
            auto h = head.load(std::memory_order_relaxed);

            BackoffPolicy policy;

            while (h == tail.load(std::memory_order_acquire)) {
                policy.pause();
            }

            auto result = std::move(buffer[h]);
            buffer[h].~T();

            head.store((h + 1) & MASK, std::memory_order_release);

            return result;
        }
    };
}
