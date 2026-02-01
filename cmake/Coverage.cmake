# ============================================================================
# Code Coverage Configuration
# ============================================================================
# Provides coverage-report target for CI integration.
# Requires gcov/lcov and debug build with coverage flags.

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# Check if coverage tools are available
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

# Only enable coverage on Debug builds with GCC/Clang
set(_coverage_supported FALSE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            set(_coverage_supported TRUE)
        endif()
    endif()
endif()

if(_coverage_supported)
    message(STATUS "Code coverage: enabled (gcov + lcov)")

    # Add coverage flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")

    # Coverage report target
    add_custom_target(coverage-report
        COMMAND ${LCOV_PATH} --capture --directory . --output-file coverage.info --ignore-errors mismatch
        COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/Qt/*' '*/test/*' '*/build/*' --output-file coverage.info
        COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage-report
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report..."
        VERBATIM
    )
else()
    message(STATUS "Code coverage: disabled (missing tools or not Debug build)")

    # Provide a no-op target so CI doesn't fail
    add_custom_target(coverage-report
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report skipped - not configured"
        COMMENT "Coverage report not available"
        VERBATIM
    )
endif()
