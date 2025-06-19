#pragma once

#include <thread>
#include <type_traits>
#include <vector>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <print>
// #include <hip/hip_runtime.h>


namespace dwhbll::linalg {

// Computation volume at which point we switch to the GPU
constexpr uint64_t GPU_VOLUME_THRESHOLD = (uint64_t) LINALG_GPU_THRESHOLD * LINALG_GPU_THRESHOLD * LINALG_GPU_THRESHOLD;

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
                data[i] = static_cast<T>(other.data[i]);
            }
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix(const Matrix<T2, Row, Col>&& other) {
            for(size_t i = 0; i < Row * Col; i++) {
                data[i] = static_cast<T>(std::move(other.data[i]));
            }
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix& operator=(const Matrix<T2, Row, Col>& other) {
            if(this != &other) {
                for(size_t i = 0; i < Row * Col; i++) {
                    data[i] = static_cast<T>(other.data[i]);
                }
            }

            return *this;
        }

        template <typename T2> requires std::convertible_to<T2, T>
        Matrix& operator=(const Matrix<T2, Row, Col>&& other) {
            if(this != &other) {
                for(size_t i = 0; i < Row * Col; i++) {
                    data[i] = static_cast<T>(std::move(other.data[i]));
                }
            }

            return *this;
        }

        ///////////////////////////////////////////////////////

        constexpr size_t rows() const { return Row; }
        constexpr size_t cols() const { return Col; }
        constexpr size_t size() const { return data.size(); }

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
        Matrix<std::common_type_t<T, T2>, Row, K> matmul(const Matrix<T2, Col, K>& other) {
            std::print("{}x{} * {}x{} > ", Row, Col, Col, K);

            using ResultType = std::common_type_t<T, T2>;
            Matrix<ResultType, Row, K> res;

            if constexpr (Row * Col * K <= GPU_VOLUME_THRESHOLD) {
                std::println("CPU");
                matmul_cpu(other, res);
            }
            else {
                std::println("too large for the CPU, dispatching to GPU [TODO]");
            }
            
            return res;
        }

        // A * B just calls the default behaviour of matmul, it's just better to read
        template<size_t K, typename T2>
        Matrix<std::common_type_t<T, T2>, Row, K> operator*(const Matrix<T2, Col, K>& other) {
            return matmul<K, T2>(other);
        }
    
    private:
        template<size_t K, typename T2>
        inline void matmul_cpu(const Matrix<T2, Col, K>& other, Matrix<T2, Col, K>& res) {

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
