#pragma once

namespace dwhbll::concurrency::coroutine {
    namespace wrappers {}
}

#include <dwhbll/macros/sanify.h>

#ifdef DWHBLL_SANIFY_EXPORT
using namespace dwhbll::concurrency::coroutine;
using namespace dwhbll::concurrency::coroutine::wrappers;
#endif
