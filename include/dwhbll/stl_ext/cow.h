#pragma once

#include <concepts>
#include <memory>

#include <dwhbll/stl_ext/packs.h>

#ifdef BUILD_HARDEN
#include <dwhbll/console/debug.hpp>
#endif

namespace dwhbll::stl_ext {
    /**
     * @brief Copy On Write wrapper, not threadsafe!
     * @tparam T Type to wrap
     */
    template <typename T>
    requires std::copy_constructible<T>
    class cow {
        std::shared_ptr<T> object;

        /**
         * @brief Makes a new object if the current object is a) not null, and b) not unique (e.g. there are other
         * classes referencing this object.)
         */
        void copy() {
            if (object && !object.unique())
                object = std::make_shared<T>(object);
        }

    public:
        template <typename... Args>
        requires (sizeof...(Args) != 1) || (!std::is_same_v<cow, template_pack_nth<0, Args...>>)
        explicit cow(Args&&... args) : object(std::forward<Args>(args)...) {}

        cow() : object(nullptr) {}

        cow(const cow &other)
            : object(other.object) {
        }

        cow(cow &&other) noexcept
            : object(std::move(other.object)) {
        }

        cow & operator=(const cow &other) {
            if (this == &other)
                return *this;
            object = other.object;
            return *this;
        }

        cow & operator=(cow &&other) noexcept {
            if (this == &other)
                return *this;
            object = std::move(other.object);
            return *this;
        }

        T const* operator->() const {
            return object.get();
        }

        T const& operator*() const {
#ifdef BUILD_HARDEN
            if (!object)
                debug::panic("Dereferencing nullptr!");
#endif
            return *object.get();
        }

        cow & operator=(const T& other) {
            object = std::make_shared<T>(other);

            return *this;
        }

        cow clone() const {
            cow result = *this;

            result.copy();

            return std::move(result);
        }
    };
}
