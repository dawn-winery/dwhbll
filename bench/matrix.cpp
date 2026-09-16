#include <dwhbll/bench/bench.h>
#include <dwhbll/linalg/matrix.h>
#include <source_location>

#include <random>

using namespace dwhbll::bench;
using namespace dwhbll::linalg;

[[=bench]]
[[=warmup(1)]]
[[=iterations(3)]]
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

    for (auto _ : state) {
        m1 * m2;
    }
}

BENCH_REGISTER_FILE();
