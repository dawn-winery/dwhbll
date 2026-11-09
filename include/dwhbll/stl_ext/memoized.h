#pragma once

#include <functional>
#include <variant>

namespace dwhbll::stl_ext {
    template <typename T>
    class memoized {
        std::variant<
            T,
            std::function<T()>
        > data;

    public:
        ~memoized() = default;

        memoized(std::function<T()> fn) : data(fn) {}

        memoized(const memoized&) = default;
        memoized(memoized&&) = default;

        memoized& operator=(const memoized&) = default;
        memoized& operator=(memoized&&) = default;

        const T& operator()() {
            if (!std::holds_alternative<T>(data))
                data = std::get<std::function<T()>>(data)();
            return std::get<T>(data);
        }
    };
}
