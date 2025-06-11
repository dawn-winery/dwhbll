#include <dwhbll/linalg/matrix.h>
#include <functional>
#include <iostream>
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
        test_failed = mat.storageSize() != (M * N);

        for(int i = 0; i < M; i++) {
            for(int j = 0; j < M; j++) {
                test_failed = mat[i,j] != 0;
            }
        }

        if(test_failed) std::println(std::cerr, "Zero initialization failed");
    }

    return test_failed;
}


std::unordered_map<std::string, std::function<bool()>> test_dispatch {
    { "init", matrix_init },
};

bool matrix_test(std::optional<std::string> test_to_run) 
{
    if(test_to_run.has_value() && test_dispatch.contains(test_to_run.value())) {
        return test_dispatch.at(test_to_run.value())(); 
    }

    return 1;
}