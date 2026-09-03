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
option(DWHBLL_SANIFY_EXPORT "Export Sanification directly to the global namespace" OFF)
option(DWHBLL_SUGAR_EXPORT "Export Sanification directly to the global namespace" OFF)

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

if (DWHBLL_SANIFY_EXPORT)
    add_compile_definitions(DWHBLL_SANIFY_EXPORT)
endif()

if (DWHBLL_SUGAR_EXPORT)
    add_compile_definitions(DWHBLL_SUGAR_EXPORT)
endif()

set(DWHBLL_SOURCES
    src/dwhbll/async/net/buffered_socket.cpp
    src/dwhbll/async/net/socket.cpp
    src/dwhbll/async/net/tcp_listener.cpp
    src/dwhbll/collections/cache.cpp
    src/dwhbll/collections/memory_buffer.cpp
    src/dwhbll/concurrency/coroutine/async_semaphore.cpp
    src/dwhbll/concurrency/coroutine/cancellable_base.cpp
    src/dwhbll/concurrency/coroutine/defer_again.cpp
    src/dwhbll/concurrency/coroutine/detached_task.cpp
    src/dwhbll/concurrency/coroutine/reactor.cpp
    src/dwhbll/concurrency/coroutine/sleep_task.cpp
    src/dwhbll/concurrency/coroutine/uring_promise.cpp
    src/dwhbll/concurrency/coroutine/uring_sqe_awaitable.cpp
    src/dwhbll/concurrency/coroutine/wrappers/file.cpp
    src/dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.cpp
    src/dwhbll/concurrency/spinlock.cpp
    src/dwhbll/console/logging.cpp
    src/dwhbll/debug/debug.cpp
    src/dwhbll/debug/panic.cpp
    src/dwhbll/debug/perf.cpp
    src/dwhbll/cryptography/arc4/arc4.cpp
    src/dwhbll/cryptography/hash/sha1.cpp
    src/dwhbll/exceptions/rt_exception_base.cpp
    src/dwhbll/files/executables/mz.cpp
    src/dwhbll/files/filejar/file.cpp
    src/dwhbll/files/filejar/file_mgr.cpp
    src/dwhbll/files/binary_file.cpp
    src/dwhbll/files/parse_utils.cpp
    src/dwhbll/graphics/bitmap.cpp
    src/dwhbll/lang/c/tokenize.cpp
    src/dwhbll/lang/common/file_stream.cpp
    src/dwhbll/lang/common.cpp
    src/dwhbll/lang/cpp/cpp_raw_stream.cpp
    src/dwhbll/lang/cpp/preprocess/line_splice_stream.cpp
    src/dwhbll/math/vec4.cpp
    src/dwhbll/network/address.cpp
    src/dwhbll/network/buffered_socket.cpp
    src/dwhbll/network/dns/dns.cpp
    src/dwhbll/network/http.cpp
    src/dwhbll/network/socket_manager.cpp
    src/dwhbll/platform/linux_wrappers/ptrace.cpp
    src/dwhbll/sanify/deferred.cpp
    src/dwhbll/subprocess/process.cpp
    src/dwhbll/subprocess/pipe_wrapper.cpp
    src/dwhbll/json/json.cpp
    src/dwhbll/stl_ext/string.cpp
    src/dwhbll/concurrency/threading.cpp
    src/dwhbll/uuid/uuid.cpp
    src/dwhbll/math/vec4.cpp

    include/dwhbll/async/net/buffered_socket.h
    include/dwhbll/async/net/decorated_socket.h
    include/dwhbll/async/net/isocket.h
    include/dwhbll/async/net/socket.h
    include/dwhbll/async/net/tcp_listener.h
    include/dwhbll/collections/cache.h
    include/dwhbll/collections/memory_buffer.h
    include/dwhbll/collections/ring.h
    include/dwhbll/collections/sorted_linked_list.h
    include/dwhbll/collections/streams.h
    include/dwhbll/concurrency/backoff/backoff_policy.h
    include/dwhbll/concurrency/backoff/policy_exponential.h
    include/dwhbll/concurrency/backoff/policy_linear.h
    include/dwhbll/concurrency/backoff/policy_pause.h
    include/dwhbll/concurrency/common.h
    include/dwhbll/concurrency/coroutine/async_semaphore.h
    include/dwhbll/concurrency/coroutine/cancellable_base.h
    include/dwhbll/concurrency/coroutine/cancellation_exception.h
    include/dwhbll/concurrency/coroutine/defer_again.h
    include/dwhbll/concurrency/coroutine/detached_task.h
    include/dwhbll/concurrency/coroutine/reactor.h
    include/dwhbll/concurrency/coroutine/sleep_task.h
    include/dwhbll/concurrency/coroutine/task.h
    include/dwhbll/concurrency/coroutine/uring_promise.h
    include/dwhbll/concurrency/coroutine/uring_sqe_awaitable.h
    include/dwhbll/concurrency/coroutine/wrappers/file.h
    include/dwhbll/concurrency/coroutine/wrappers/syscall_wrappers.h
    include/dwhbll/concurrency/queues/bounded_mpsc_queue.h
    include/dwhbll/concurrency/queues/bounded_spsc_queue.h
    include/dwhbll/concurrency/recycling_concurrent_stack.h
    include/dwhbll/concurrency/spinlock.h
    include/dwhbll/console/ansi_escape.h
    include/dwhbll/debug/debug.h
    include/dwhbll/debug/panic.h
    include/dwhbll/debug/perf.h
    include/dwhbll/debug/stacktrace.h
    include/dwhbll/console/logging.h
    include/dwhbll/cryptography/arc4/arc4.h
    include/dwhbll/cryptography/hmac.h
    include/dwhbll/cryptography/ihash.h
    include/dwhbll/cryptography/hash/sha1.h
    include/dwhbll/exceptions/concurrency_exception.h
    include/dwhbll/exceptions/rt_exception_base.h
    include/dwhbll/exceptions/sys_error.h
    include/dwhbll/exceptions/timeout_exception.h
    include/dwhbll/files/executables/mz.h
    include/dwhbll/files/filejar/file.h
    include/dwhbll/files/filejar/file_mgr.h
    include/dwhbll/files/binary_file.h
    include/dwhbll/files/parse_utils.h
    include/dwhbll/graphics/bitmap/bitmap.h
    include/dwhbll/graphics/bitmap/file_header.h
    include/dwhbll/graphics/bitmap/info_header.h
    include/dwhbll/graphics/pixel.h
    include/dwhbll/lang/c/tokenize.h
    include/dwhbll/lang/common/file_stream.h
    include/dwhbll/lang/common/stream.h
    include/dwhbll/lang/common.h
    include/dwhbll/lang/cpp/common.h
    include/dwhbll/lang/cpp/cpp_raw_stream.h
    include/dwhbll/lang/cpp/preprocess/line_splice_stream.h
    include/dwhbll/linalg/matrix.h
    include/dwhbll/math/pad_box.h
    include/dwhbll/math/rect.h
    include/dwhbll/memory/pool.h
    include/dwhbll/network/address.h
    include/dwhbll/network/buffered_socket.h
    include/dwhbll/network/dns/dns.h
    include/dwhbll/network/http/methods.h
    include/dwhbll/network/http/versions.h
    include/dwhbll/network/http.h
    include/dwhbll/network/http_server.h
    include/dwhbll/network/socket_manager.h
    include/dwhbll/platform/linux_wrappers/ptrace.h
    include/dwhbll/sanify/all.h
    include/dwhbll/sanify/coroutines.h
    include/dwhbll/sanify/deferred.h
    include/dwhbll/sanify/lang_ext.h
    include/dwhbll/sanify/stl_ext.h
    include/dwhbll/sanify/sugar/all.h
    include/dwhbll/sanify/sugar/ranges.h
    include/dwhbll/sanify/types.h
    include/dwhbll/stl_ext/common_helpers.h
    include/dwhbll/stl_ext/cow.h
    include/dwhbll/stl_ext/get_set.h
    include/dwhbll/stl_ext/option.h
    include/dwhbll/stl_ext/packs.h
    include/dwhbll/stl_ext/result.h
    include/dwhbll/stl_ext/templates.h
    include/dwhbll/subprocess/pipe_wrapper.h
    include/dwhbll/subprocess/process.h
    include/dwhbll/debug/format.h
    include/dwhbll/meta/refl.h
    include/dwhbll/json/json.h
    include/dwhbll/stl_ext/string.h
    include/dwhbll/concurrency/threading.h
    include/dwhbll/uuid/uuid.h
    include/dwhbll/math/vec4.h
    include/dwhbll/concurrency/owning_spinlock.h
    include/dwhbll/stl_ext/ranges.h
    include/dwhbll/stl_ext/utilities.h
    include/dwhbll/stl_ext/try.h
    include/dwhbll/unicode/table.h
)

# Autogenerated C++ for unicode attributes
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(UCD_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/data/unicode"
)

set(UCD_ZIP
    "${UCD_DIR}/UCD.zip"
)

set(UNICODE_TABLES
    "${CMAKE_CURRENT_BINARY_DIR}/generated/unicode_tables.cpp"
)

set(DOWNLOAD_UCD
    "${CMAKE_CURRENT_SOURCE_DIR}/script/unicode/fetch.py"
)

set(GENERATE_UNICODE
    "${CMAKE_CURRENT_SOURCE_DIR}/script/unicode/generate_tables.py"
)

add_custom_command(
    OUTPUT
    "${UCD_ZIP}"

    COMMAND
    "${Python3_EXECUTABLE}"
    "${DOWNLOAD_UCD}"
    "${UCD_ZIP}"

    DEPENDS
    "${DOWNLOAD_UCD}"

    COMMENT
    "Downloading Unicode Character Database"
    VERBATIM
)

add_custom_command(
    OUTPUT
    "${UNICODE_TABLES}"

    COMMAND
    "${Python3_EXECUTABLE}"
    "${GENERATE_UNICODE}"
    "${UCD_ZIP}"
    "${UNICODE_TABLES}"

    DEPENDS
    "${UCD_ZIP}"
    "${GENERATE_UNICODE}"

    COMMENT
    "Generating Unicode tables"
    VERBATIM
)

set(DWHBLL_AUTOGENERATED_SOURCES
    ${UNICODE_TABLES}
    include/dwhbll/files/executables/pe.h
    src/dwhbll/files/executables/pe.cpp
)

# Main target
add_library(dwhbll INTERFACE)

# Core library, contains most things
add_library(dwhbll_core STATIC ${DWHBLL_SOURCES} ${DWHBLL_AUTOGENERATED_SOURCES})
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
        tests/graphics/bitmap.cpp
        tests/lang/c/tokenizer_test.cpp
        tests/bench/bounded_spsc_int_bench.cpp
        tests/bench/bounded_mpsc_int_bench.cpp
        tests/bench/recycling_concurrent_stack_bench.cpp
        tests/cryptography/arc4.cpp
    )

    add_executable(dwhbll_test ${DWHBLL_TEST_CXX_SOURCES})


    target_link_libraries(dwhbll_test PRIVATE dwhbll)
    target_include_directories(dwhbll_test PRIVATE include)
endif()

add_executable(http_server_test tests/http_server_test.cpp)
target_link_libraries(http_server_test dwhbll)
target_include_directories(http_server_test PRIVATE include)
