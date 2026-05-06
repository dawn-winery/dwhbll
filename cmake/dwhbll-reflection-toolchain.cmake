# cmake/dwhbll-reflection-toolchain.cmake

set(GCC_ROOT "/opt/gcc" CACHE PATH "Custom toolchain path")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_COMPILER "${GCC_ROOT}/bin/gcc" CACHE FILEPATH "Custom gcc path" FORCE)
set(CMAKE_CXX_COMPILER "${GCC_ROOT}/bin/g++" CACHE FILEPATH "Custom g++ path" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection" CACHE STRING "Set reflection")

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    "${GCC_ROOT}/include/c++/16.0.0"
    "${GCC_ROOT}/include/c++/16.0.0/x86_64-pc-linux-gnu"
)

set(CMAKE_BUILD_RPATH "${GCC_ROOT}/lib64")
set(CMAKE_INSTALL_RPATH "${GCC_ROOT}/lib64")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

cmake_policy(SET CMP0079 NEW)
