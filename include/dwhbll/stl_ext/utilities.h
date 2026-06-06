#pragma once
#include <dwhbll/sanify/deferred.h>

namespace dwhbll::stl_ext {
    /**
     * @brief Set a value temporarily, returning it to the original value at the end of the scope
     * @tparam T Type of the value to change
     * @param storage Value to change
     * @param value Value to temporarily set it to
     * @return Re-setter object.
     */
    template <typename T, typename U>
    [[nodiscard]] constexpr sanify::deferred store_temporary(T& storage, U &&value) {
        T tmp = std::move(storage);
        storage = std::forward<U>(value);

        return sanify::deferred([tmp = std::move(tmp), &storage]() -> void {
            storage = std::move(tmp);
        });
    }
}
