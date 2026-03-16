#pragma once

namespace dwhbll::concurrency::backoff {
    template <typename T>
    concept BackoffPolicy = requires(T a)
    {
        a.reset();
        a.pause();
    };
}
