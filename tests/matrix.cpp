#include <dwhbll/testing/testing.h>
#include <dwhbll/linalg/matrix.h>

#include <cstdlib>
#include <print>
#include <random>

using namespace dwhbll::test;
using namespace dwhbll::linalg;

namespace matrix {

[[=test]]
void zero_init()
{
    constexpr int M = 500;
    constexpr int N = 300;

    dwhbll::linalg::Matrix<int, M, N> mat;
    REQUIRE(mat.size() == (M * N));

    for(int i = 0; i < M; i++)
        for(int j = 0; j < N; j++)
            REQUIRE((mat[i,j] == 0));
}

[[=test]]
void init_list()
{
    dwhbll::linalg::Matrix<int, 3, 3> mat {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8
    };

    REQUIRE(mat.size() == 9);

    for(size_t i = 0; i < 3; i++)
        for(size_t j = 0; j < 3; j++)
            REQUIRE((mat[i,j] == static_cast<int>(i * mat.rows() + j)));
}

[[=test]]
void square_matrix()
{
    dwhbll::linalg::Matrix<int, 4, 4> m1 {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    };

    dwhbll::linalg::Matrix<double, 4, 4> m2 {
        16, 15, 14, 13,
        12, 11, 10, 9,
        8, 7, 6, 5,
        4, 3, 2, 1
    };

    dwhbll::linalg::Matrix<int, 4, 4> expected {
        80, 70, 60, 50,
        240, 214, 188, 162,
        400, 358, 316, 274,
        560, 502, 444, 386
    };

    auto res = m1 * m2;
    REQUIRE(res.rows() == m1.rows());
    REQUIRE(res.cols() == m2.cols());

    for(size_t i = 0; i < res.rows(); i++)
        for(size_t j = 0; j < res.cols(); j++)
            REQUIRE((res[i,j] == expected[i,j]));
}

[[=test]]
void matmul()
{
    constexpr size_t SIZE = 512;
    Matrix<int, SIZE, SIZE> m1;
    Matrix<int, SIZE, SIZE> m2;

    std::mt19937_64 generator(17);
    std::uniform_int_distribution<int> distrib(0, 128);

    for(size_t i = 0; i < SIZE; i++) {
        for(size_t j = 0; j < SIZE; j++) {
            m1[i,j] = distrib(generator);
            m2[i,j] = distrib(generator);
        }
    }

    auto res = m1 * m2;
    auto res_cpu = m1.matmul<SIZE, int, dwhbll::linalg::ExecutionPolicy::CPU>(m2);
    float tolerance = 1e-5;

    for(size_t i = 0; i < SIZE; i++)
        for(size_t j = 0; j < SIZE; j++)
            REQUIRE(std::abs(res[i,j] - res_cpu[i,j]) <= tolerance);
}

}

TEST_REGISTER_FILE();
