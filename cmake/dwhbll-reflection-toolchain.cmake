# cmake/dwhbll-reflection-toolchain.cmake

set(LLVM_ROOT "/opt/llvm-reflect" CACHE PATH "Custom LLVM install path with Clang and libc++")

set(CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang" CACHE FILEPATH "Custom Clang C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${LLVM_ROOT}/bin/clang++" CACHE FILEPATH "Custom Clang++ compiler" FORCE)

set(USE_LIBCPP "ON" CACHE STRING "Force use of libc++")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++" CACHE STRING "Use libc++ and no system includes")

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    "${LLVM_ROOT}/include/c++/v1"
    "${LLVM_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1"
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${LLVM_ROOT}/lib -Wl,-rpath,${LLVM_ROOT}/lib/x86_64-unknown-linux-gnu")

cmake_policy(SET CMP0079 NEW)
