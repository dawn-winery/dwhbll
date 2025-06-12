#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>

namespace dwhbll::linealg {

template <typename T, size_t Row, size_t Col> requires std::default_initializable<T>
class Matrix {
    private:
        std::array<T, Row * Col> data;
    
    public:
        Matrix() : data{}
        {
        }

        template<typename... Args> requires (sizeof...(Args) == Row * Col && (std::convertible_to<Args, T> && ...))
        Matrix(Args... d)
        {
            size_t i = 0;
            ((data[i++] = static_cast<T>(d)), ...);
        }

        constexpr size_t rowCount() const { return Row; }
        constexpr size_t colCount() const { return Col; }
        constexpr size_t storageSize() const { return data.size(); }

        T& operator[](size_t row, size_t col)
        {
            assert(row < Row && col < Col);
            return data[row * Col + col];
        }

        template<size_t K>
        Matrix<T, Row, K> operator*(const Matrix<T, Col, K>& other)
        {
            Matrix<T, Row, K> res;

            for(size_t row = 0; row < Row; row++) {
                for(size_t col = 0; col < K; col++) {
                    for(size_t cur = 0; cur < Col; cur++) {
                        res[row][col] += data[row][cur] * other[cur][col];
                    }
                }
            }

            return res;
        }
};



}
