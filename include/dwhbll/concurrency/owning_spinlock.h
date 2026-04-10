#pragma once

#include <memory>

#include <dwhbll/concurrency/spinlock.h>

namespace dwhbll::concurrency {
    template <typename T>
    class owning_spinlock {
        std::unique_ptr<spinlock> lock_t;

        T object;

    public:
        template <typename... Args>
        owning_spinlock(Args&&... args) : object(std::forward<Args>(args)...), lock_t(std::make_unique<spinlock>()) {}

        owning_spinlock(const owning_spinlock &other) = delete;

        owning_spinlock(owning_spinlock &&other) noexcept
            : lock_t(std::move(other.lock_t)),
              object(std::move(other.object)) {
        }

        owning_spinlock & operator=(const owning_spinlock &other) = delete;

        owning_spinlock & operator=(owning_spinlock &&other) noexcept {
            if (this == &other)
                return *this;
            lock_t = std::move(other.lock_t);
            object = std::move(other.object);
            return *this;
        }

        class lock_result {
            owning_spinlock* owner;

            sanify::deferred cleanup;

            explicit lock_result(owning_spinlock* parent) : owner(parent), cleanup(parent->lock_t->lock()) {}

        public:
            lock_result(const lock_result &other) = delete;

            lock_result(lock_result &&other) noexcept
                : owner(other.owner), cleanup(std::move(other.cleanup)) {
            }

            lock_result & operator=(const lock_result &other) = delete;

            lock_result & operator=(lock_result &&other) noexcept {
                if (this == &other)
                    return *this;
                owner = other.owner;
                cleanup = std::move(other.cleanup);
                return *this;
            }

            friend class owning_spinlock;

            T* operator->() {
                return &(owner->object);
            }

            T& operator*() {
                return owner->object;
            }

            T& get() {
                return owner->object;
            }
        };

        [[nodiscard]] lock_result lock() {
            return lock_result{this};
        }
    };
}
