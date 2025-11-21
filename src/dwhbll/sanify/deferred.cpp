#include <dwhbll/sanify/deferred.h>

namespace dwhbll::sanify {
    deferred::deferred(const std::function<void()> &deferred_function): deferred_function(deferred_function) {}

    deferred::~deferred() {
        if (deferred_function)
            deferred_function();
    }

    deferred::deferred(deferred &&other) noexcept: deferred_function(std::move(other.deferred_function)) {
        other.deferred_function = nullptr;
    }

    deferred & deferred::operator=(deferred &&other) noexcept {
        if (this == &other)
            return *this;
        deferred_function = std::move(other.deferred_function);
        other.deferred_function = nullptr;
        return *this;
    }

    void swap(deferred &lhs, deferred &rhs) noexcept {
        using std::swap;
        swap(lhs.deferred_function, rhs.deferred_function);
    }
}
