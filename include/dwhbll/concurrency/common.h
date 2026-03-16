#pragma once

#include <new>

namespace dwhbll::concurrency {
#if __cpp_lib_hardware_interference_size >= 201703L
    constexpr std::size_t AlignmentSize = std::hardware_destructive_interference_size;
#else
    constexpr std::size_t AlignmentSize = 64;
#endif
}
