#pragma once


#include <cstdint>
namespace dwhbll::linalg {


constexpr uint64_t GPU_VOLUME_THRESHOLD = (uint64_t) LINALG_GPU_THRESHOLD * LINALG_GPU_THRESHOLD * LINALG_GPU_THRESHOLD;


enum class ExecutionPolicy {
    Auto,
    CPU,
    GPU,
    HIP
};


}