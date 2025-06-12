#include <chrono>
#include <dwhbll/linalg/matrix.h>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <print>
#include <random>
#include <string>
#include <optional>
#include <unordered_map>

bool matrix_init() 
{
    bool test_failed = false;

    // Test zero init
    {
        constexpr int M = 5;
        constexpr int N = 5;

        dwhbll::linealg::Matrix<int, M, N> mat;
        test_failed |= mat.storageSize() != (M * N);
        if(test_failed) std::println(std::cerr, "[Zero init] Failed to allocated enough storage");

        for(int i = 0; i < M; i++) {
            for(int j = 0; j < M; j++) {
                test_failed |= mat[i,j] != 0;
            }
        }

        if(test_failed)
            std::println(std::cerr, "[Zero init] Initialization failed");
    }

    // Test init list
    {
        dwhbll::linealg::Matrix<int, 3, 3> mat {
            0, 1, 2,
            3, 4, 5,
            6, 7, 8
        };

        test_failed |= mat.storageSize() != 9;
        if(test_failed)
            std::println(std::cerr, "[Init list] Failed to allocated enough storage");


        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                test_failed |= mat[i,j] != i * mat.rowCount() + j;
            }
        }

        if(test_failed)
            std::println(std::cerr, "[Init list] Initialization failed");
    }

    return test_failed;
}

bool matrix_mul() {

    bool test_failed = false;

    // Square matrix
    {
        dwhbll::linealg::Matrix<int, 4, 4> m1 {
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 15, 16
        };

        dwhbll::linealg::Matrix<int, 4, 4> m2 {
            16, 15, 14, 13,
            12, 11, 10, 9,
            8, 7, 6, 5,
            4, 3, 2, 1
        };

        dwhbll::linealg::Matrix<int, 4, 4> expected {
            80, 70, 60, 50,
            240, 214, 188, 162,
            400, 358, 316, 274,
            560, 502, 444, 386
        };

        auto res = m1 * m2;
        test_failed |= (res.rowCount() != m1.rowCount() || res.colCount() != m2.colCount());
        if(test_failed) 
            std::println(std::cerr, "[Square] Result matrix of wrong dimensions. Expected {}x{}, got {}x{}", 
                m1.rowCount(), m2.colCount(), 
                res.rowCount(), res.colCount()
            );

        for(int i = 0; i < res.rowCount(); i++) {
            for(int j = 0; j < res.colCount(); j++) {
                bool failed = (res[i,j] != expected[i,j]);
                test_failed |= failed;
                if(failed)
                    std::println(std::cerr, "[Square] Multiplication failed at [{},{}]. Expected {}, got {}", i, j, expected[i,j], res[i,j]);
            }
        }
    }

    {
        constexpr size_t SIZE = 8192;
        dwhbll::linealg::Matrix<int, SIZE, SIZE> m1;
        dwhbll::linealg::Matrix<int, SIZE, SIZE> m2;

        std::random_device rd;
        std::mt19937_64 generator(rd());
        std::uniform_int_distribution<int> distrib(0, 128);

        for(auto i = 0; i < SIZE; i++) {
            for(auto j = 0; j < SIZE; j++) {
                m1[i,j] = distrib(generator);
                m2[i,j] = distrib(generator); 
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        auto res = m1 * m2;
        auto t2 = std::chrono::high_resolution_clock::now();
        std::println(std::cout, "Mat mul of matrices {}x{} took {}", SIZE, SIZE, std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
    }

    return test_failed;
}

std::unordered_map<std::string, std::function<bool()>> test_dispatch {
    { "init", matrix_init },
    { "mul", matrix_mul }
};

bool matrix_test(std::optional<std::string> test_to_run) 
{
    if(test_to_run.has_value() && test_dispatch.contains(test_to_run.value())) {
        return test_dispatch.at(test_to_run.value())(); 
    }

    return 1;
}
