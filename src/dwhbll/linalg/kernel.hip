
#ifdef DWHBLL_GPU_ENABLED

#include <cassert>
#include <dwhbll/linalg/matrix.h>
#include <hip/hip_runtime.h>
#include <iostream>

#define HIP_CHECK(command) {                                                \
    hipError_t status = command;                                            \
    if (status != hipSuccess) {                                             \
        std::cerr << "HIP Error: " << hipGetErrorString(status)             \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl;    \
        exit(status);                                                       \
    }                                                                       \
}


namespace dwhbll::linalg {

template <typename T>
__global__ void matmul(const T* A, const T* B, T* out, int M, int N, int K) {
    // Calculate global row and column indices for this thread
    int row = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
    int col = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;

    // Perform bounds checking to ensure thread operates within matrix dimensions
    if (row < M && col < N) {
        T sum = 0.0;
        // Compute the dot product of A's row and B's column
        for (int p = 0; p < K; ++p) {
            sum += A[row * K + p] * B[p * N + col];
        }
        out[row * N + col] = sum;
    }
}



template <typename ComputeT>
std::vector<ComputeT> matmul_gpu_wrapper(const std::vector<ComputeT>& A, const std::vector<ComputeT>& B, 
                                         std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB) 
{
    
    auto [rowA, colA] = dimA;
    auto [rowB, colB] = dimB;
    assert(colA == rowB);
    
    // const ComputeT *h_A = static_cast<const ComputeT*>(matA);
    // const ComputeT *h_B = static_cast<const ComputeT*>(matB);
    size_t s = rowA * colB * sizeof(ComputeT);
    std::vector<ComputeT> out;
    out.reserve(rowA * colB);

    ComputeT *d_A, *d_B, *d_res;

    HIP_CHECK(hipMalloc(&d_A, A.size() * sizeof(ComputeT)));
    HIP_CHECK(hipMalloc(&d_B, B.size() * sizeof(ComputeT)));
    HIP_CHECK(hipMalloc(&d_res, out.capacity() * sizeof(ComputeT)));

    HIP_CHECK(hipMemcpy(d_A , A.data(), A.size() * sizeof(ComputeT), hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(d_B , B.data(), B.size() * sizeof(ComputeT), hipMemcpyHostToDevice));


    dim3 dimBlock(16,16);
    dim3 dimGrid((colB + dimBlock.x - 1) / dimBlock.x,
                 (rowA + dimBlock.y - 1) / dimBlock.y);


    std::println("Launching kernel with dtype={}", typeid(ComputeT).name());
    matmul<ComputeT><<<dimGrid, dimBlock>>>(d_A, d_B, d_res, rowA, colA, colB);

    HIP_CHECK(hipDeviceSynchronize());
    HIP_CHECK(hipMemcpy(out.data(), d_res, out.capacity() * sizeof(ComputeT), hipMemcpyDeviceToHost));

    HIP_CHECK(hipFree(d_A));
    HIP_CHECK(hipFree(d_B));
    HIP_CHECK(hipFree(d_res));

    return out;

}


template std::vector<float> matmul_gpu_wrapper<float>(const std::vector<float>& A, const std::vector<float>& B, 
                                                      std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB);

template std::vector<double> matmul_gpu_wrapper<double>(const std::vector<double>& A, const std::vector<double>& B, 
                                                        std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB);

}

#endif