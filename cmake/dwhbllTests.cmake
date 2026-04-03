include_guard(GLOBAL)

set(_DWHBLL_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})  

function(dwhbll_add_tests TARGET_NAME)
    set(TEST_FILES ${ARGN})
    if (NOT TEST_FILES)
        message(FATAL_ERROR "Missing test files")
    endif()

    set(ABS_TEST_FILES "")
    foreach(f ${TEST_FILES})
        get_filename_component(abs_f ${f} ABSOLUTE)
        list(APPEND ABS_TEST_FILES ${abs_f})
    endforeach()

    find_package(Python3 REQUIRED)

    set(GENERATOR_SCRIPT "${_DWHBLL_CMAKE_DIR}/find_module_exports.py")
    set(RUNNER_TEMPLATE "${_DWHBLL_CMAKE_DIR}/test_runner.cpp")
    set(GENERATED_RUNNER "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_gen.cpp")

    add_custom_command(
        OUTPUT ${GENERATED_RUNNER}
        COMMAND Python3::Interpreter
            ${GENERATOR_SCRIPT}
            ${RUNNER_TEMPLATE}
            ${GENERATED_RUNNER}
            ${ABS_TEST_FILES}
        DEPENDS
            ${GENERATOR_SCRIPT}
            ${RUNNER_TEMPLATE}
            ${ABS_TEST_FILES}
        COMMENT "Generating test runner"
        VERBATIM
    )

    add_library(${TARGET_NAME}_modules STATIC)
    target_sources(${TARGET_NAME}_modules PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${ABS_TEST_FILES}
    )
    target_link_libraries(${TARGET_NAME}_modules PUBLIC dwhbll::dwhbll)

    add_executable(${TARGET_NAME} ${GENERATED_RUNNER})
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_modules)

    target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME}_modules)
endfunction()
