#pragma once

#include <dwhbll/stl_ext/result.h>

namespace dwhbll::stl_ext {
    template <typename T>
    using errno_result = Result<T, int>;
}

#ifdef DWHBLL_SANIFY_EXPORT
using namespace dwhbll::stl_ext;
#endif
