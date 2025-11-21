#pragma once

#include <functional>

namespace dwhbll::sanify {
    class deferred {
        std::function<void()> deferred_function;

    public:
        [[nodiscard]] explicit deferred(const std::function<void()> &deferred_function);

        ~deferred();

        deferred(const deferred &other) = delete;

        deferred(deferred &&other) noexcept;

        deferred & operator=(const deferred &other) = delete;

        deferred & operator=(deferred &&other) noexcept;

        friend void swap(deferred &lhs, deferred &rhs) noexcept;
    };
}
