#pragma once

#include <thread>
#include <vector>
#include <cassert>
#include <concepts>
#include <cstddef>

namespace dwhbll::linealg {

template <typename T, size_t Row, size_t Col> requires std::default_initializable<T>
class Matrix {
    private:
        std::vector<T> data;
    
    public:
        Matrix() : data(Row * Col)
        {
        }

        template<typename... Args> requires (sizeof...(Args) == Row * Col && (std::convertible_to<Args, T> && ...))
        Matrix(Args&&... d) : data { static_cast<T>(std::forward<Args>(d))... }
        {
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
        Matrix<T, Row, K> operator*(Matrix<T, Col, K>& other)
        {
            Matrix<T, Row, K> res;

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

            std::vector<std::jthread> threads;
            
            for(int i = 0; i < num_threads; i++) {
                size_t start_row = i * row_per_threads;
                size_t end_row = start_row + row_per_threads;

                if (i == num_threads - 1) {
                    end_row += remaining_rows;
                }

                threads.emplace_back(MatmulWorker, start_row, end_row);
            }


            return res;
        }
};



}
