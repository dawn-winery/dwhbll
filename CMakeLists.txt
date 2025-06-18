cmake_minimum_required(VERSION 3.31)
project(dwhbll LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

option(BUILD_HARDEN "enable hardening on the library." OFF)

if (BUILD_HARDEN)
    add_compile_definitions(DWHBLL_HARDEN)
endif()

add_library(dwhbll STATIC
        include/dwhbll/memory/pool.h
        include/dwhbll/collections/ring.h
        include/dwhbll/network/dns/dns.h
        include/dwhbll/network/SocketManager.h
        include/dwhbll/console/Logging.h
        src/dwhbll/network/console/Logging.cpp
        src/dwhbll/network/dns/dns.cpp
        src/dwhbll/network/SocketManager.cpp
)
set_target_properties(dwhbll PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(dwhbll PRIVATE include)

add_executable(dwhbll_test
        tests/test_main.cpp
        tests/pool.cpp
        tests/matrix.cpp
        tests/collections/ring.cpp
)
target_link_libraries(dwhbll_test PRIVATE dwhbll)
target_include_directories(dwhbll_test PRIVATE include)
