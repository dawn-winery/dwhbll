#pragma once

namespace dwhbll::concurrency::coroutine {
    namespace wrappers {}
}

#define yield co_yield
#define finish co_return

#ifdef DWHBLL_SANIFY_EXPORT
using namespace dwhbll::concurrency::coroutine;
using namespace dwhbll::concurrency::coroutine::wrappers;
#endif
