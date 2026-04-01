#pragma once

#include <functional>

namespace dwhbll::stl_ext {
    /**
     * @brief Wraps a pair of functions that can be used as both a value source and drain
     * Basically only useful for porting c# getter/setter pairs to c++
     * @tparam T the type of the getter/setter
     */
    template <typename T>
    class get_set {
        std::function<T()> _get;
        std::function<void(T&&)> _set;

    public:
        get_set(const std::function<T()>& get, const std::function<void(T&&)>& set) : _get(get), _set(set) {}

        get_set& operator=(T&& value) {
            _set(value);

            return *this;
        }

        operator T() {
            return std::move(_get());
        }
    };
}
