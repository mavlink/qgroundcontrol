# Code coverage configuration for QGroundControl
# Enabled via: cmake -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#
# Targets:
#   coverage-report  - Generate XML + HTML reports (for CI/Codecov)
#   coverage-html    - Generate HTML report only (for local viewing)
#   coverage-clean   - Remove coverage data files

if(NOT QGC_ENABLE_COVERAGE)
    return()
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage requires Debug build, but CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    return()
endif()

message(STATUS "Code coverage instrumentation enabled")

# Compiler-specific coverage flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Using GCC coverage (gcov/lcov)")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang source-based coverage (llvm-cov)")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(WARNING "Code coverage not supported for MSVC. Use Visual Studio Enterprise or OpenCppCoverage.")
    return()

else()
    message(WARNING "Code coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    return()
endif()

# Find gcovr for report generation
find_program(GCOVR_EXECUTABLE gcovr)

if(GCOVR_EXECUTABLE)
    message(STATUS "Found gcovr: ${GCOVR_EXECUTABLE}")

    # Common gcovr arguments
    set(GCOVR_COMMON_ARGS
        --root ${CMAKE_SOURCE_DIR}
        --filter ${CMAKE_SOURCE_DIR}/src/
        --exclude ".*moc_.*"
        --exclude ".*qrc_.*"
        --exclude ".*ui_.*"
        --exclude ".*_autogen.*"
        --print-summary
    )

    # coverage-report: Generate both XML (for Codecov) and HTML
    add_custom_target(coverage-report
        COMMAND ${GCOVR_EXECUTABLE}
            ${GCOVR_COMMON_ARGS}
            --xml coverage.xml
            --html coverage.html
            --html-details
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report (XML + HTML)"
        VERBATIM
    )

    # coverage-html: Generate HTML only (for local viewing)
    add_custom_target(coverage-html
        COMMAND ${GCOVR_EXECUTABLE}
            ${GCOVR_COMMON_ARGS}
            --html coverage.html
            --html-details
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating HTML coverage report"
        VERBATIM
    )

    # coverage-clean: Remove coverage data files
    add_custom_target(coverage-clean
        COMMAND ${CMAKE_COMMAND} -E rm -f coverage.xml coverage.html
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete 2>/dev/null || true
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning coverage data"
        VERBATIM
    )

else()
    message(STATUS "gcovr not found - coverage report targets not available")
    message(STATUS "  Install with: pip install gcovr")
endif()
