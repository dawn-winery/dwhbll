cmake_minimum_required(VERSION 3.31)
project(dwhbll LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

add_library(dwhbll STATIC
        include/dwhbll/memory/pool.h
)
set_target_properties(dwhbll PROPERTIES LINKER_LANGUAGE CXX)

add_executable(dwhbll_test
        tests/test_main.cpp
        tests/pool.cpp
)
target_link_libraries(dwhbll_test PRIVATE dwhbll)
target_include_directories(dwhbll_test PRIVATE include)
