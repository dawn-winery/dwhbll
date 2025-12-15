cmake_minimum_required(VERSION 3.31)
project(dwhbll LANGUAGES C CXX)

include(CheckCXXSymbolExists)

find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

option(DWHBLL_TESTS "Build tests" OFF)
option(BUILD_HARDEN "enable hardening on the library." OFF)
option(BUILD_HARDEN_EXPENSIVE "enable expensive hardening features on the library." OFF)
option(DWHBLL_GPU_ENABLED "Toggle GPU acceleration" ON)

set(LINALG_GPU_THRESHOLD_VALUE 2048 CACHE STRING "Select the threshold for GPU delegation")

check_cxx_symbol_exists(_LIBCPP_VERSION version LIBCPP)
if(LIBCPP)
    message(FATAL_ERROR "libc++ is not supported")
endif()

if (BUILD_HARDEN)
    add_compile_definitions(DWHBLL_HARDEN)
endif()
if (BUILD_HARDEN_EXPENSIVE)
    add_compile_definitions(DWHBLL_HARDEN_EXPENSIVE)
endif()


set(DWHBLL_SOURCES
    src/dwhbll/collections/cache.cpp
    src/dwhbll/collections/memory_buffer.cpp
    src/dwhbll/concurrency/coroutine/async_semaphore.cpp
    src/dwhbll/concurrency/coroutine/defer_again.cpp
    src/dwhbll/concurrency/coroutine/detached_task.cpp
    src/dwhbll/concurrency/coroutine/reactor.cpp
    src/dwhbll/concurrency/coroutine/sleep_task.cpp
    src/dwhbll/concurrency/coroutine/uring_promise.cpp
    src/dwhbll/concurrency/coroutine/uring_sqe_awaitable.cpp
    src/dwhbll/concurrency/coroutine/wrappers/file.cpp
    src/dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.cpp
    src/dwhbll/concurrency/spinlock.cpp
    src/dwhbll/console/Logging.cpp
    src/dwhbll/console/debug.cpp
    src/dwhbll/cryptography/SHA1.cpp
    src/dwhbll/exceptions/rt_exception_base.cpp
    src/dwhbll/files/parse_utils.cpp
    src/dwhbll/graphics/bitmap.cpp
    src/dwhbll/network/buffered_socket.cpp
    src/dwhbll/network/dns/dns.cpp
    src/dwhbll/network/http.cpp
    src/dwhbll/network/SocketManager.cpp
    src/dwhbll/sanify/deferred.cpp
    src/dwhbll/subprocess/process.cpp
    src/dwhbll/subprocess/pipe_wrapper.cpp
    src/dwhbll/utils/json.cpp
    src/dwhbll/utils/string.cpp
    src/dwhbll/utils/utils.cpp

    include/dwhbll/collections/cache.h
    include/dwhbll/collections/memory_buffer.h
    include/dwhbll/collections/ring.h
    include/dwhbll/collections/sorted_linked_list.h
    include/dwhbll/collections/streams.hpp
    include/dwhbll/concurrency/coroutine/async_semaphore.h
    include/dwhbll/concurrency/coroutine/defer_again.h
    include/dwhbll/concurrency/coroutine/detached_task.h
    include/dwhbll/concurrency/coroutine/reactor.h
    include/dwhbll/concurrency/coroutine/sleep_task.h
    include/dwhbll/concurrency/coroutine/task.h
    include/dwhbll/concurrency/coroutine/uring_promise.h
    include/dwhbll/concurrency/coroutine/uring_sqe_awaitable.h
    include/dwhbll/concurrency/coroutine/wrappers/file.h
    include/dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h
    include/dwhbll/concurrency/spinlock.h
    include/dwhbll/console/debug.hpp
    include/dwhbll/console/Logging.h
    include/dwhbll/cryptography/banner.h
    include/dwhbll/cryptography/SHA1.h
    include/dwhbll/exceptions/concurrency_exception.h
    include/dwhbll/exceptions/rt_exception_base.h
    include/dwhbll/exceptions/sys_error.h
    include/dwhbll/exceptions/timeout_exception.h
    include/dwhbll/files/parse_utils.h
    include/dwhbll/graphics/bitmap/bitmap.h
    include/dwhbll/graphics/bitmap/file_header.h
    include/dwhbll/graphics/bitmap/info_header.h
    include/dwhbll/graphics/pixel.h
    include/dwhbll/linalg/matrix.h
    include/dwhbll/math/pad_box.h
    include/dwhbll/math/rect.h
    include/dwhbll/memory/pool.h
    include/dwhbll/network/buffered_socket.h
    include/dwhbll/network/dns/dns.h
    include/dwhbll/network/http.h
    include/dwhbll/network/SocketManager.h
    include/dwhbll/sanify/coroutines.hpp
    include/dwhbll/sanify/deferred.h
    include/dwhbll/sanify/types.hpp
    include/dwhbll/subprocess/pipe_wrapper.h
    include/dwhbll/subprocess/process.h
    include/dwhbll/utils/format.hpp
    include/dwhbll/utils/json.hpp
    include/dwhbll/utils/stacktrace.hpp
    include/dwhbll/utils/string.h
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

target_link_libraries(dwhbll_core PUBLIC stdc++exp)

install(TARGETS dwhbll_core
    EXPORT dwhbllTargets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

# find liburing
find_package(PkgConfig REQUIRED)
pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)
target_link_libraries(dwhbll_core PRIVATE PkgConfig::uring)

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
