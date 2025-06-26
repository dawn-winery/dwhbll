#ifdef DWHBLL_GPU_ENABLED

#include <cassert>
#include <dwhbll/linalg/matrix.h>
#include <hip/hip_runtime.h>

namespace dwhbll::linalg {

template <typename T>
__global__ void helloWorldKernel() {
    // This kernel simply prints "Hello from device!"
    // In a real kernel, you would perform parallel computation here.
    // printf() inside a kernel is generally for debugging and can have performance implications.
    printf("Hello from device with type! (Thread: %d, Block: %d)\n", hipThreadIdx_x, hipBlockIdx_x);
}



template <typename ComputeT>
void matmul_gpu_wrapper(const void *matA, const void *matB, void *res, std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB) {
    auto [rowA, colA] = dimA;
    auto [rowB, colB] = dimB;

    assert(colA == rowB);

    // std::vector<ComputeT> matA_T(rowA * colA);
    // std::vector<ComputeT> matB_T(rowB * colB);

    std::println("Launching kernel with dtype={}", typeid(ComputeT).name());
    helloWorldKernel<ComputeT><<<dim3(1), dim3(256)>>>();
}

template void matmul_gpu_wrapper<float>(const void *matA, const void *matB, void *res, std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB);
template void matmul_gpu_wrapper<double>(const void *matA, const void *matB, void *res, std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB);

}

#endif