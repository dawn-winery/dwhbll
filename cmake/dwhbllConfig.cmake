include(CMakeFindDependencyMacro)
find_dependency(PkgConfig)
pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)

include("${CMAKE_CURRENT_LIST_DIR}/dwhbllTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/dwhbllTests.cmake")
