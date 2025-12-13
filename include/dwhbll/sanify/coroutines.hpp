#pragma once

namespace dwhbll::concurrency::coroutine {
    namespace wrappers {}
}

#define yield co_yield
#define await co_await
#define finish co_return

using namespace dwhbll::concurrency::coroutine;
using namespace dwhbll::concurrency::coroutine::wrappers;
