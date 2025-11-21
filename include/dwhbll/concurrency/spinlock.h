#pragma once

#include <atomic>

#include <dwhbll/sanify/deferred.h>

namespace dwhbll::concurrency {
    class spinlock {
        std::atomic_flag _lock;

    public:
        [[nodiscard]] spinlock() = default;

        ~spinlock();

        spinlock(const spinlock &other) = delete;

        spinlock(spinlock &&other) noexcept = delete;

        spinlock & operator=(const spinlock &other) = delete;

        spinlock & operator=(spinlock &&other) noexcept = delete;

        /**
         * @brief Locks the spinlock
         * @return Returns a deferred object to unlock the lock later, the spinlock must be kept alive until the deferred object runs!
         */
        [[nodiscard]] sanify::deferred lock();
    };
}
