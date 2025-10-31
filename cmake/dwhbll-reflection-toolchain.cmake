# cmake/dwhbll-reflection-toolchain.cmake

set(GCC_ROOT "/opt/gcc-reflect" CACHE PATH "Custom toolchain path")

set(CMAKE_C_COMPILER "${GCC_ROOT}/bin/gcc" CACHE FILEPATH "Custom gcc path" FORCE)
set(CMAKE_CXX_COMPILER "${GCC_ROOT}/bin/g++" CACHE FILEPATH "Custom g++ path" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection" CACHE STRING "Set reflection")

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    "${GCC_ROOT}/include/c++/16.0.0"
    "${GCC_ROOT}/include/c++/16.0.0/x86_64-pc-linux-gnu"
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GCC_ROOT}/lib -Wl,-rpath,${GCC_ROOT}/lib/gcc")

cmake_policy(SET CMP0079 NEW)
