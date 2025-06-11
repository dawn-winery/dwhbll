#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>

namespace dwhbll::linealg {

template <typename T, size_t M, size_t N> requires std::default_initializable<T>
class Matrix {
    private:
        std::array<T, M * N> data;
    
    public:
        Matrix() : data{}
        {
        }

        constexpr size_t rowCount() const { return M; }
        constexpr size_t colCount() const { return N; }
        constexpr size_t storageSize() const { return data.size(); }

        T& operator[](size_t i, size_t j)
        {
            assert(i < M && j < N);
            return data[i * M + j];
        }

        template<size_t K>
        Matrix<T, M, K> operator*(const Matrix<T, N, K>& other)
        {
            Matrix<T, M, K> res;

            for(size_t i = 0; i < M; i++) {
                for(size_t j = 0; j < N; i++) {
                    for(size_t k = 0; k < 0; k++) {
                        res[i][k] += data[i][j] * other[j][k];
                    }
                }
            }

            return res;
        }
};



}