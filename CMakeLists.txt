cmake_minimum_required(VERSION 3.31)
project(dwhbll LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

option(BUILD_HARDEN "enable hardening on the library." OFF)
option(BUILD_HARDEN_EXPENSIVE "enable expensive hardening features on the library." OFF)
option(DWHBLL_GPU_ENABLED "Toggle GPU acceleration" ON)
set(LINALG_GPU_THRESHOLD_VALUE 2048 CACHE STRING "Select the threshold for GPU delegation")

if (BUILD_HARDEN)
    add_compile_definitions(DWHBLL_HARDEN)
endif()
if (BUILD_HARDEN_EXPENSIVE)
    add_compile_definitions(DWHBLL_HARDEN_EXPENSIVE)
endif()


set(DWHBLL_SOURCES
    include/dwhbll/memory/pool.h
    include/dwhbll/collections/ring.h
    include/dwhbll/network/dns/dns.h
    include/dwhbll/network/SocketManager.h
    include/dwhbll/console/Logging.h
    include/dwhbll/linalg/matrix.h
    include/dwhbll/collections/cache.h
    src/dwhbll/console/Logging.cpp
    src/dwhbll/network/dns/dns.cpp
    src/dwhbll/network/SocketManager.cpp
    src/dwhbll/collections/cache.cpp
)

# Main target
add_library(dwhbll INTERFACE)

# Core library, contains most things
add_library(dwhbll_core STATIC ${DWHBLL_SOURCES})
set_target_properties(dwhbll_core PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(dwhbll_core PUBLIC include)

# Propagates the linker to the core library if targetting the main target
target_link_libraries(dwhbll INTERFACE dwhbll_core)

# Conditional defining of the GPU accelerated library
# This is because otherwise CMake propagates the HIP
# compile arguments to all files, and it tries linking
# cpp files with the HIP toolchain, which isn't available for them
if (DWHBLL_GPU_ENABLED)
    message(STATUS "GPU acceleration requested. Searching for HIP...")
    find_package(HIP QUIET)
    if (NOT HIP_FOUND)
        message("HIP not found. GPU support disabled.")
    else()
        message(STATUS "HIP found at ${ROCM_PATH}")

        enable_language(HIP)
        set(CMAKE_HIP_STANDARD 23)


        add_compile_definitions(LINALG_GPU_THRESHOLD=${LINALG_GPU_THRESHOLD_VALUE})
        add_compile_definitions(DWHBLL_GPU_ENABLED)

        set(DWHBLL_HIP_SOURCES
                src/dwhbll/linalg/kernel.hip
        )

        add_library(dwhbll_gpu STATIC ${DWHBLL_HIP_SOURCES})
        target_include_directories(dwhbll_gpu PRIVATE include)

        target_link_libraries(dwhbll_gpu PRIVATE hip::device)

        
        target_link_libraries(dwhbll INTERFACE dwhbll_gpu)
    endif()
else()
    message(STATUS "Disabling GPU acceleration...")
endif()



# TODO : IMPROVE INTEGRATION OF GPU COMPUTE FOR USERS 

set(DWHBLL_TEST_CXX_SOURCES
    tests/test_main.cpp
    tests/pool.cpp
    tests/collections/ring.cpp
    tests/matrix.cpp
    tests/collections/cache.cpp
)

add_executable(dwhbll_test ${DWHBLL_TEST_CXX_SOURCES})


target_link_libraries(dwhbll_test PRIVATE dwhbll)
target_include_directories(dwhbll_test PRIVATE include)



# TODO : CREATE CMAKE PACKAGE FOR EASY INTEGRATION WITH CMAKE AND MESON
