cmake_minimum_required(VERSION 3.31)
project(dwhbll LANGUAGES C CXX)

include(CheckCXXSymbolExists)

find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

option(DWHBLL_TESTS "Build tests" ON)
option(BUILD_HARDEN "enable hardening on the library." OFF)
option(BUILD_HARDEN_EXPENSIVE "enable expensive hardening features on the library." OFF)
option(DWHBLL_GPU_ENABLED "Toggle GPU acceleration" ON)
option(DWHBLL_REFLECTION_ENABLED "Enable reflection and use custom Clang with libc++" OFF)
set(USE_LIBCPP "" CACHE STRING "Use libc++ or libstdc++ [\"ON\", \"OFF\", or \"\" to use default]")
set(LINALG_GPU_THRESHOLD_VALUE 2048 CACHE STRING "Select the threshold for GPU delegation")

if(NOT USE_LIBCPP STREQUAL "ON" AND NOT USE_LIBCPP STREQUAL "OFF" AND NOT USE_LIBCPP STREQUAL "")
    message(FATAL_ERROR "USE_LIBCPP must be 'ON', 'OFF', or empty")
endif()

check_cxx_symbol_exists(_LIBCPP_VERSION version DEFAULT_LIBCPP)
if(USE_LIBCPP STREQUAL "")
    if(DEFAULT_LIBCPP)
        set(USE_LIBCPP "ON")
    else()
        set(USE_LIBCPP "OFF")
    endif()
endif()

if(USE_LIBCPP STREQUAL "OFF" AND DWHBLL_REFLECTION_ENABLED)
    message(FATAL_ERROR "Reflection can't be used with libstdc++")
endif()

if (BUILD_HARDEN)
    add_compile_definitions(DWHBLL_HARDEN)
endif()
if (BUILD_HARDEN_EXPENSIVE)
    add_compile_definitions(DWHBLL_HARDEN_EXPENSIVE)
endif()


set(DWHBLL_SOURCES
    src/dwhbll/collections/cache.cpp
    src/dwhbll/console/Logging.cpp
    src/dwhbll/console/debug.cpp
    src/dwhbll/exceptions/rt_exception_base.cpp
    src/dwhbll/graphics/bitmap/bitmap.cpp
    src/dwhbll/network/dns/dns.cpp
    src/dwhbll/network/SocketManager.cpp
    src/dwhbll/subprocess/process.cpp
    src/dwhbll/subprocess/pipe_wrapper.cpp
    src/dwhbll/utils/json.cpp
    src/dwhbll/utils/utils.cpp

    include/dwhbll/collections/cache.h
    include/dwhbll/collections/ring.h
    include/dwhbll/collections/streams.hpp
    include/dwhbll/console/debug.hpp
    include/dwhbll/console/Logging.h
    include/dwhbll/exceptions/rt_exception_base.h
    include/dwhbll/exceptions/sys_error.h
    include/dwhbll/exceptions/timeout_exception.h
    include/dwhbll/graphics/bitmap/bitmap.h
    include/dwhbll/graphics/bitmap/file_header.h
    include/dwhbll/graphics/bitmap/info_header.h
    include/dwhbll/graphics/pixel.h
    include/dwhbll/linalg/matrix.h
    include/dwhbll/math/pad_box.h
    include/dwhbll/math/rect.h
    include/dwhbll/memory/pool.h
    include/dwhbll/network/dns/dns.h
    include/dwhbll/network/SocketManager.h
    include/dwhbll/sanify/coroutines.hpp
    include/dwhbll/sanify/types.hpp
    include/dwhbll/subprocess/pipe_wrapper.h
    include/dwhbll/subprocess/process.h
    include/dwhbll/utils/format.hpp
    include/dwhbll/utils/json.hpp
    include/dwhbll/utils/stacktrace.hpp
    include/dwhbll/utils/utils.hpp
)

# Main target
add_library(dwhbll INTERFACE)

# Core library, contains most things
add_library(dwhbll_core STATIC ${DWHBLL_SOURCES})
set_target_properties(dwhbll_core PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(dwhbll_core
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# If reflection is enabled, try to find a clang-p2996 install dir and use that to build
if (DWHBLL_REFLECTION_ENABLED)
    if(NOT DEFINED LLVM_ROOT)
        message(FATAL_ERROR "LLVM_ROOT must be defined when building with reflection support. Use the dwhbll-reflection-toolchain.cmake.")
    endif()

    if (NOT EXISTS "${CMAKE_CXX_COMPILER}")
        message(FATAL_ERROR "Custom Clang++ not found at ${CMAKE_CXX_COMPILER}")
    endif()

    message(STATUS "Building with reflection support")

    target_compile_definitions(dwhbll_core PUBLIC DWHBLL_REFLECTION)

    target_compile_options(dwhbll_core PUBLIC
        -freflection-latest
        -fexpansion-statements
    )

    target_include_directories(dwhbll_core PUBLIC
        ${LLVM_ROOT}/include/c++/v1
        ${LLVM_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
    )

    target_link_options(dwhbll_core PUBLIC
        -L${LLVM_ROOT}/lib
        -Wl,-rpath,${LLVM_ROOT}/lib
    )
endif()


if(USE_LIBCPP STREQUAL "OFF")
    message(STATUS "Using libstdc++")

    #target_link_options(dwhbll_core PUBLIC "-stdlib=libstdc++")
    target_link_libraries(dwhbll_core PUBLIC stdc++exp)
else()
    message(STATUS "Using libc++")

    target_link_options(dwhbll_core PUBLIC "-stdlib=libc++")

    pkg_check_modules(LIBDW REQUIRED libdw)
    pkg_check_modules(LIBELF REQUIRED libelf)

    target_include_directories(dwhbll_core PUBLIC
        ${LIBDW_INCLUDE_DIRS}
        ${LIBELF_INCLUDE_DIRS}
    )

    target_link_libraries(dwhbll_core PUBLIC
        ${LIBDW_LIBRARIES}
        ${LIBELF_LIBRARIES}
        dl
    )
endif()


install(TARGETS dwhbll_core
    EXPORT dwhbllTargets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

# Propagates the linker to the core library if targetting the main target
target_link_libraries(dwhbll INTERFACE dwhbll_core)

# Conditional defining of the GPU accelerated library
# This is because otherwise CMake propagates the HIP
# compile arguments to all files, and it tries linking
# cpp files with the HIP toolchain, which isn't available for them
if (DWHBLL_GPU_ENABLED)
    message(STATUS "GPU acceleration requested. Searching for HIP...")
    find_package(HIP QUIET)
    if (HIP_FOUND)
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

        install(TARGETS dwhbll_gpu
            EXPORT dwhbllTargets
            ARCHIVE DESTINATION lib
            INCLUDES DESTINATION include
        )

        target_link_libraries(dwhbll INTERFACE dwhbll_gpu)
    else()
        message(WARNING "HIP not found. GPU support disabled.")
    endif()
else()
    message(STATUS "Disabling GPU acceleration...")
endif()

install(DIRECTORY include/ DESTINATION include)

install(TARGETS dwhbll
    EXPORT dwhbllTargets
)

install(EXPORT dwhbllTargets
    FILE dwhbllTargets.cmake
    NAMESPACE dwhbll::
    DESTINATION lib/cmake/dwhbll
)

# Version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/dwhbllConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dwhbllConfig.cmake" # needed for find_package()
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dwhbll-reflection-toolchain.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dwhbllConfigVersion.cmake"
    DESTINATION lib/cmake/dwhbll
)

# TODO : IMPROVE INTEGRATION OF GPU COMPUTE FOR USERS

if (DWHBLL_TESTS)
    set(DWHBLL_TEST_CXX_SOURCES
        tests/test_main.cpp
        tests/pool.cpp
        tests/matrix.cpp
        tests/collections/cache.cpp
        tests/collections/ring.cpp
        tests/collections/streams.cpp
    )

    add_executable(dwhbll_test ${DWHBLL_TEST_CXX_SOURCES})


    target_link_libraries(dwhbll_test PRIVATE dwhbll)
    target_include_directories(dwhbll_test PRIVATE include)
endif()
