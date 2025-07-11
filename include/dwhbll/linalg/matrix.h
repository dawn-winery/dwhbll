#pragma once

#include <algorithm>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <print>



namespace dwhbll::linalg {



#ifdef DWHBLL_GPU_ENABLED
// Computation volume at which point we switch to the GPU
constexpr uint64_t GPU_VOLUME_THRESHOLD = (uint64_t) LINALG_GPU_THRESHOLD * LINALG_GPU_THRESHOLD * LINALG_GPU_THRESHOLD;

template <typename ComputeT>
std::vector<ComputeT> matmul_gpu_wrapper(const std::vector<ComputeT>& A, const std::vector<ComputeT>& B, 
                                         std::pair<size_t, size_t> dimA, std::pair<size_t, size_t> dimB);


#endif

enum class ExecutionPolicy {
    Auto,
    CPU,
    GPU,
    HIP
};


// These concepts ensure a Matrix element is convertible to any arithmetic type
// and have arithmetic operations defined. This ensures that, if the class compiles,
// subsequent matrix operations are guaranteed to work

template <typename T, typename T2>
concept Arithmetic = requires (T t, T2 s) {
    { t + s } -> std::convertible_to<std::common_type_t<T, T2>>;
    { t - s } -> std::convertible_to<std::common_type_t<T, T2>>;
    { t * s } -> std::convertible_to<std::common_type_t<T, T2>>;
    { s + t } -> std::convertible_to<std::common_type_t<T, T2>>;
    { s - t } -> std::convertible_to<std::common_type_t<T, T2>>;
    { s * t } -> std::convertible_to<std::common_type_t<T, T2>>;
};

template <typename T>
concept MatrixElement =
    std::default_initializable<T> &&
    Arithmetic<T, T> &&
    Arithmetic<T, int> &&
    Arithmetic<T, double> &&
    Arithmetic<T, float> &&
    Arithmetic<T, long>;




template <MatrixElement T, size_t Row, size_t Col>
class Matrix {
    private:

        // Internal storage of the matrix. Flat storage in row major order.
        std::vector<T> data;

    public:

        using value_type = T;

        Matrix() : data(Row * Col) {}
        Matrix(const Matrix& other) = default;
        Matrix(Matrix&& other) noexcept = default;

        Matrix& operator=(const Matrix& other) = default;
        Matrix& operator=(Matrix&& other) noexcept = default;

        ~Matrix() = default;

        // Variadic constructor to allow construction from a list of elements
        // Assumes row major order
        template<typename... Args> requires (sizeof...(Args) == Row * Col && (std::convertible_to<Args, T> && ...))
        Matrix(Args&&... d) : data { static_cast<T>(std::forward<Args>(d))... } {}

        // Copy and move constructors and assignment operators
        // For when matrix B has a different type than matrix A
        template <typename T2> requires std::convertible_to<T2, T>
        Matrix(const Matrix<T2, Row, Col>& other) {
            for(size_t i = 0; i < Row * Col; i++) {
                data[i] = static_cast<T>(other.raw_handle()[i]);
            }
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix(const Matrix<T2, Row, Col>&& other) noexcept {
            for(size_t i = 0; i < Row * Col; i++) {
                data[i] = static_cast<T>(std::move(other.data[i]));
            }
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix& operator=(Matrix<T2, Row, Col>& other) {
            if(this != &other) {
                for(size_t i = 0; i < Row * Col; i++) {
                    data[i] = static_cast<T>(other.raw_handle()[i]);
                }
            }

            return *this;
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix& operator=(Matrix<T2, Row, Col>&& other) noexcept {
            if(this != &other) {
                for(size_t i = 0; i < Row * Col; i++) {
                    data[i] = static_cast<T>(std::move(other.data[i]));
                }
            }

            return *this;
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix& operator=(T2* other) noexcept {
            for(size_t i = 0; i < Row * Col; i++) {
                data[i] = static_cast<T>(std::move(other[i]));
            }
            return *this;
        }

        ///////////////////////////////////////////////////////

        constexpr size_t rows() const { return Row; }
        constexpr size_t cols() const { return Col; }
        constexpr size_t size() const { return data.size(); }
        const T* raw_handle() const { return data.data(); }

        T& operator[](size_t row, size_t col) {
            assert(row < Row && col < Col);
            return data[row * Col + col];
        }

        const T& operator[](size_t row, size_t col) const {
            assert(row < Row && col < Col);
            return data[row * Col + col];
        }

        // TODO : +, -, / operators

        // Main matmul function. Responsible for dispatching between the CPU and the GPU
        // TODO: make custom GPU kernels and a hipBLAS implementation
        template<size_t K, typename T2, ExecutionPolicy Policy = ExecutionPolicy::Auto>
        requires MatrixElement<T2> && Arithmetic<T, T2>
        Matrix<std::common_type_t<T, T2>, Row, K> matmul(const Matrix<T2, Col, K>& other) const {
            std::print("{}x{} * {}x{} > ", Row, Col, Col, K);

            using ResultType = std::common_type_t<T, T2>;
            Matrix<ResultType, Row, K> res;

#ifdef DWHBLL_GPU_ENABLED
            if constexpr (Policy == ExecutionPolicy::CPU) {
                std::println("CPU");
                matmul_cpu<K, T2, ResultType>(other, res);
            }
            else if constexpr (Policy == ExecutionPolicy::GPU) {
                // TODO
            }
            else if constexpr (Policy == ExecutionPolicy::HIP) {
                std::println("Using hipBLAS GPU kernel [TODO]");
            }
            else {
                // ExecutionPolicy::Auto
                if constexpr (Row * Col * K <= GPU_VOLUME_THRESHOLD) {
                    std::println("Auto dispatch to CPU");
                    matmul_cpu<K, T2, ResultType>(other, res);
                }
                else {
                    std::println("Auto dispatch to GPU");

                    std::vector<double> A(this->size()), B(other.size());
                    std::ranges::transform(this->data, A.begin(), [] (int i) { return static_cast<double>(i); });
                    std::ranges::transform(other.data, B.begin(), [] (int i) { return static_cast<double>(i); });

                    auto bleh = matmul_gpu_wrapper<double>(A, B, std::make_pair(Row, Col), std::make_pair(Col, K));
                    res = std::move(bleh.data());
                }

            }
#else
            matmul_cpu<K, T2, ResultType>(other, res);
#endif
            return res;
        }

        // A * B just calls the default behaviour of matmul, it's just better to read
        template<size_t K, typename T2>
        Matrix<std::common_type_t<T, T2>, Row, K> operator*(const Matrix<T2, Col, K>& other) const {
            return matmul<K, T2>(other);
        }

    private:
        template<size_t K, typename T2, typename ResultType>
        inline void matmul_cpu(const Matrix<T2, Col, K>& other, Matrix<ResultType, Col, K>& res) const {

            auto MatmulWorker = [&] (size_t start_row, size_t end_row) {
                for(size_t row = start_row; row < end_row; row++) {
                    for(size_t cur = 0; cur < Col; cur++) {
                        for(size_t col = 0; col < K; col++) {
                            res[row, col] += (*this)[row, cur] * other[cur, col];
                        }
                    }
                }
            };

            size_t num_threads = std::thread::hardware_concurrency();
            if(num_threads == 0) num_threads = 1;   // Fallback for safety
            size_t row_per_threads = Row / num_threads;
            size_t remaining_rows  = Row % num_threads;

            {
                std::vector<std::jthread> threads;

                for(int i = 0; i < num_threads; i++) {
                    size_t start_row = i * row_per_threads;
                    size_t end_row = start_row + row_per_threads;

                    if (i == num_threads - 1) {
                        end_row += remaining_rows;
                    }

                    threads.emplace_back(MatmulWorker, start_row, end_row);
                }
            }
        }
};


}
