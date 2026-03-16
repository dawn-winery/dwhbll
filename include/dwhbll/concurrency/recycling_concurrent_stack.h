#pragma once

#include <atomic>
#include <concepts>
#include <optional>

#include <dwhbll/concurrency/common.h>

namespace dwhbll::concurrency {
    /**
     * @brief Trades a moderate loss of performance for recycling stack objects,
     * @tparam T
     */
    template <typename T>
    requires std::default_initializable<T>
    class RecyclingConcurrentStack {
        struct Node {
            Node* next;
            T data;
        };

        alignas(AlignmentSize) std::atomic<Node*> _head = nullptr;
        alignas(AlignmentSize) std::atomic<Node*> _free = nullptr;

    public:
        RecyclingConcurrentStack() = default;

        RecyclingConcurrentStack(std::size_t initial_capacity) {
            Node* last = nullptr;

            for (int i = 0; i < initial_capacity; i++)
                last = new Node(last);

            _free.store(last, std::memory_order_release);
        }

        [[nodiscard]] constexpr bool empty() const {
            return _head.load(std::memory_order_relaxed) == nullptr;
        }

        template <typename... Args>
        void push(Args&&... args) {
            // fetch new node from the list
            Node* node_;
            Node* next_;

            do {
                node_ = _free.load(std::memory_order_relaxed);

                if (!node_)
                    break;

                next_ = node_->next;
            } while (!_free.compare_exchange_weak(node_, next_, std::memory_order_release, std::memory_order_relaxed));

            if (!node_)
                node_ = new Node;

            new (&node_->data) T(std::forward<Args...>(args...));

            do {
                next_ = _head.load(std::memory_order_relaxed);
                node_->next = next_;
            } while (!_head.compare_exchange_weak(next_, node_, std::memory_order_release, std::memory_order_relaxed));
        }

        std::optional<T> pop() {
            Node* node_;
            Node* next_;

            do {
                node_ = _head.load(std::memory_order_relaxed);
                if (node_ == nullptr)
                    return std::nullopt;
                next_ = node_->next;
            } while (!_head.compare_exchange_weak(node_, next_, std::memory_order_release, std::memory_order_relaxed));

            std::optional<T> result = std::move(node_->data);
            node_->data.~T();

            do {
                next_ = _free.load(std::memory_order_relaxed);
                node_->next = next_;
            } while (!_free.compare_exchange_weak(next_, node_, std::memory_order_release, std::memory_order_relaxed));

            return result;
        }
    };
}
