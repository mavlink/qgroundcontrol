# Code coverage configuration for QGroundControl
# Enabled via: cmake -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#
# Targets:
#   coverage         - Run tests and generate XML + HTML reports
#   coverage-report  - Generate XML + HTML reports from existing coverage data
#   coverage-html    - Run tests and generate HTML report only
#   coverage-clean   - Remove coverage data files
#
# Prerequisites:
#   - gcovr (pip install gcovr)
#   - For GCC: gcov (usually installed with gcc)
#   - For Clang: llvm-cov (usually installed with clang)

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
    set(GCOVR_GCOV_EXECUTABLE gcov)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang source-based coverage (llvm-cov)")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate)

    # Find llvm-cov and create gcov wrapper
    find_program(LLVM_COV_PATH llvm-cov)
    if(LLVM_COV_PATH)
        set(GCOVR_GCOV_EXECUTABLE "${LLVM_COV_PATH} gcov")
    endif()

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
    message(STATUS "Coverage thresholds: lines=${QGC_COVERAGE_LINE_THRESHOLD}%, branches=${QGC_COVERAGE_BRANCH_THRESHOLD}%")

    # Common gcovr arguments
    set(GCOVR_COMMON_ARGS
        --root ${CMAKE_SOURCE_DIR}
        --filter ${CMAKE_SOURCE_DIR}/src/
        --exclude ".*moc_.*"
        --exclude ".*qrc_.*"
        --exclude ".*ui_.*"
        --exclude ".*_autogen.*"
        --exclude ".*/cpm_modules/.*"
        --exclude ".*/build/.*"
        --print-summary
    )

    # Add gcov executable if found
    if(GCOVR_GCOV_EXECUTABLE)
        list(APPEND GCOVR_COMMON_ARGS --gcov-executable "${GCOVR_GCOV_EXECUTABLE}")
    endif()

    if(QGC_BUILD_TESTING)
        # coverage-report: Generate reports from existing coverage data only.
        # Intended for CI flows that already ran tests separately.
        add_custom_target(coverage-report
            COMMAND ${GCOVR_EXECUTABLE}
                ${GCOVR_COMMON_ARGS}
                --xml coverage.xml
                --html coverage.html
                --html-details
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report from existing coverage data (XML + HTML)"
            VERBATIM
        )

        # coverage: Run tests and generate both XML (for Codecov) and HTML
        add_custom_target(coverage
            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L Unit
            # Generate reports
            COMMAND ${GCOVR_EXECUTABLE}
                ${GCOVR_COMMON_ARGS}
                --xml coverage.xml
                --html coverage.html
                --html-details
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and generating coverage report (XML + HTML)"
            VERBATIM
        )
        add_dependencies(coverage ${CMAKE_PROJECT_NAME})

        # coverage-html: Run tests and generate HTML only (for local viewing)
        add_custom_target(coverage-html
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L Unit
            COMMAND ${GCOVR_EXECUTABLE}
                ${GCOVR_COMMON_ARGS}
                --html coverage.html
                --html-details
            COMMAND ${CMAKE_COMMAND} -E echo "Coverage report: ${CMAKE_BINARY_DIR}/coverage.html"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and generating HTML coverage report"
            VERBATIM
        )
        add_dependencies(coverage-html ${CMAKE_PROJECT_NAME})

        # coverage-check: Run tests and verify coverage meets thresholds (for CI)
        add_custom_target(coverage-check
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L Unit
            COMMAND ${GCOVR_EXECUTABLE}
                ${GCOVR_COMMON_ARGS}
                --fail-under-line ${QGC_COVERAGE_LINE_THRESHOLD}
                --fail-under-branch ${QGC_COVERAGE_BRANCH_THRESHOLD}
                --xml coverage.xml
                --html coverage.html
                --html-details
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and verifying coverage thresholds (lines>=${QGC_COVERAGE_LINE_THRESHOLD}%, branches>=${QGC_COVERAGE_BRANCH_THRESHOLD}%)"
            VERBATIM
        )
        add_dependencies(coverage-check ${CMAKE_PROJECT_NAME})
    endif()

    # coverage-clean: Remove coverage data files
    add_custom_target(coverage-clean
        COMMAND ${CMAKE_COMMAND} -E rm -f coverage.xml coverage.html
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.profraw" -delete
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.profdata" -delete
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning coverage data"
        VERBATIM
    )

else()
    message(STATUS "gcovr not found - coverage report targets not available")
    message(STATUS "  Install with: pip install gcovr")
endif()
