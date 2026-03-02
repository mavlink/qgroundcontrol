# Code coverage configuration for QGroundControl
# Enabled via: cmake -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#
# Targets:
#   coverage         - Run tests and generate XML + HTML reports
#   coverage-report  - Generate XML + HTML reports from existing coverage data
#   coverage-check   - Verify thresholds against existing coverage data
#   coverage-clean   - Remove coverage data files
#
# Prerequisites:
#   - gcovr (pip install gcovr)
#   - For GCC: gcov (usually installed with gcc)
#   - For Clang: llvm-cov (usually installed with clang)

include_guard(GLOBAL)

if(NOT QGC_ENABLE_COVERAGE)
    return()
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage requires Debug build, but CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    return()
endif()

message(STATUS "Code coverage instrumentation enabled")

# Disable compiler caching for coverage builds. ccache does not cache .gcno
# files (a side effect of --coverage), so cache hits produce .o files without
# the corresponding .gcno, causing gcovr to report 0% coverage.
# The CACHE FORCE affects future targets; set_property updates the existing target
# whose launcher property was initialized at qt_add_executable() time.
set(CMAKE_C_COMPILER_LAUNCHER "" CACHE STRING "C compiler launcher" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER "" CACHE STRING "CXX compiler launcher" FORCE)
if(TARGET ${CMAKE_PROJECT_NAME})
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY C_COMPILER_LAUNCHER "")
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_COMPILER_LAUNCHER "")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Using GCC coverage (gcov/lcov)")
    # Scope coverage flags to the QGC target only. Global add_compile_options
    # would leak --coverage into CPM third-party static libraries, injecting
    # __gcov_* symbols that cause linker errors.
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage)
    set(GCOVR_GCOV_EXECUTABLE gcov)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang source-based coverage (llvm-cov)")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate)

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

find_program(GCOVR_EXECUTABLE gcovr)

if(GCOVR_EXECUTABLE)
    message(STATUS "Found gcovr: ${GCOVR_EXECUTABLE}")
    message(STATUS "Coverage thresholds: lines=${QGC_COVERAGE_LINE_THRESHOLD}%, branches=${QGC_COVERAGE_BRANCH_THRESHOLD}%")

    # gcovr 8.x prepends CWD to relative filters, which breaks out-of-source builds
    set(GCOVR_COMMON_ARGS
        --root ${CMAKE_SOURCE_DIR}
        --object-directory ${CMAKE_BINARY_DIR}
        --filter "${CMAKE_SOURCE_DIR}/src/"
        --filter "${CMAKE_SOURCE_DIR}/test/"
        --exclude ".*moc_.*"
        --exclude ".*qrc_.*"
        --exclude ".*ui_.*"
        --exclude ".*_autogen.*"
        --exclude ".*/cpm_modules/.*"
        --exclude ".*/_deps/.*"
        --print-summary
    )

    if(GCOVR_GCOV_EXECUTABLE)
        list(APPEND GCOVR_COMMON_ARGS --gcov-executable "${GCOVR_GCOV_EXECUTABLE}")
    endif()

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

    if(QGC_BUILD_TESTING)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L Unit
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

        add_custom_target(coverage-check
            COMMAND ${GCOVR_EXECUTABLE}
                ${GCOVR_COMMON_ARGS}
                --fail-under-line ${QGC_COVERAGE_LINE_THRESHOLD}
                --fail-under-branch ${QGC_COVERAGE_BRANCH_THRESHOLD}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Verifying coverage thresholds (lines>=${QGC_COVERAGE_LINE_THRESHOLD}%, branches>=${QGC_COVERAGE_BRANCH_THRESHOLD}%) — run 'coverage' target first"
            VERBATIM
        )
    endif()

    add_custom_target(coverage-clean
        COMMAND ${CMAKE_COMMAND} -E rm -f coverage.xml coverage.html
        COMMAND find ${CMAKE_BINARY_DIR} "(" -name "*.gcda" -o -name "*.profraw" -o -name "*.profdata" ")" -delete
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning coverage data"
        VERBATIM
    )

else()
    message(STATUS "gcovr not found - coverage report targets not available")
    message(STATUS "  Install with: pip install gcovr")
endif()
