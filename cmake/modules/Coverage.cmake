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

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR
        "QGC: Code coverage requires a Debug build, but CMAKE_BUILD_TYPE is '${CMAKE_BUILD_TYPE}'")
endif()

foreach(_threshold IN ITEMS QGC_COVERAGE_LINE_THRESHOLD QGC_COVERAGE_BRANCH_THRESHOLD)
    if(NOT "${${_threshold}}" MATCHES "^([0-9]|[1-9][0-9]|100)$")
        message(FATAL_ERROR "${_threshold} must be an integer from 0 through 100")
    endif()
endforeach()

message(STATUS "Code coverage instrumentation enabled")

# Disable compiler caching for coverage builds. ccache does not cache .gcno
# files (a side effect of --coverage), so cache hits produce .o files without
# the corresponding .gcno, causing gcovr to report 0% coverage.
# The CACHE FORCE affects future targets; set_property updates the existing target
# whose launcher property was initialized at qt_add_executable() time.
set(CMAKE_C_COMPILER_LAUNCHER "" CACHE STRING "C compiler launcher" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER "" CACHE STRING "CXX compiler launcher" FORCE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(_qgc_coverage_compile_options --coverage -O0 -g)
    set(_qgc_coverage_link_options --coverage)
    string(REGEX MATCH "^[0-9]+" _qgc_gcc_major "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(_qgc_gcov_executable NAMES "gcov-${_qgc_gcc_major}" gcov NO_CACHE)
    set(GCOVR_GCOV_EXECUTABLE "${_qgc_gcov_executable}")
    message(STATUS "Using GCC coverage (${GCOVR_GCOV_EXECUTABLE})")
    unset(_qgc_gcc_major)
    unset(_qgc_gcov_executable)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang source-based coverage (llvm-cov)")
    set(_qgc_coverage_compile_options -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    set(_qgc_coverage_link_options -fprofile-instr-generate)

    find_program(LLVM_COV_PATH llvm-cov)
    if(LLVM_COV_PATH)
        set(GCOVR_GCOV_EXECUTABLE "${LLVM_COV_PATH} gcov")
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(WARNING "QGC: Code coverage not supported for MSVC. Use Visual Studio Enterprise or OpenCppCoverage.")
    return()

else()
    message(WARNING "QGC: Code coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    return()
endif()

# Apply coverage to each project-owned compiled target. Static and object
# libraries need instrumentation at compile time; the final executable carries
# the coverage runtime at link time. Multi-config builds instrument Debug only.
function(qgc_apply_coverage_to_target target)
    if(NOT target OR NOT TARGET ${target})
        message(FATAL_ERROR "QGC: qgc_apply_coverage_to_target: Target '${target}' does not exist")
    endif()

    get_target_property(_target_type ${target} TYPE)
    get_target_property(_imported ${target} IMPORTED)
    if(_imported OR _target_type STREQUAL "INTERFACE_LIBRARY" OR _target_type STREQUAL "UTILITY")
        message(FATAL_ERROR
            "QGC: qgc_apply_coverage_to_target: '${target}' must be a non-imported compiled target")
    endif()

    set_property(TARGET ${target} PROPERTY C_COMPILER_LAUNCHER "")
    set_property(TARGET ${target} PROPERTY CXX_COMPILER_LAUNCHER "")
    foreach(_option IN LISTS _qgc_coverage_compile_options)
        target_compile_options(${target} PRIVATE "$<$<CONFIG:Debug>:${_option}>")
    endforeach()
    if(_target_type MATCHES "^(EXECUTABLE|SHARED_LIBRARY|MODULE_LIBRARY)$")
        foreach(_option IN LISTS _qgc_coverage_link_options)
            target_link_options(${target} PRIVATE "$<$<CONFIG:Debug>:${_option}>")
        endforeach()
    endif()
endfunction()

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
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # GCC can emit negative branch counters for exception-heavy code (GCC PR 68080).
        list(APPEND GCOVR_COMMON_ARGS --gcov-ignore-parse-errors=negative_hits.warn_once_per_file)
    endif()

    add_custom_target(coverage-report
        COMMAND "${GCOVR_EXECUTABLE}"
            ${GCOVR_COMMON_ARGS}
            --xml coverage.xml
            --html coverage.html
            --html-details
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Generating coverage report from existing coverage data (XML + HTML)"
        VERBATIM
    )

    if(QGC_BUILD_TESTING)
        add_custom_target(coverage
            COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" --output-on-failure -L Unit
            COMMAND "${GCOVR_EXECUTABLE}"
                ${GCOVR_COMMON_ARGS}
                --xml coverage.xml
                --html coverage.html
                --html-details
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Running tests and generating coverage report (XML + HTML)"
            VERBATIM
        )
        add_dependencies(coverage ${CMAKE_PROJECT_NAME})

        add_custom_target(coverage-check
            COMMAND "${GCOVR_EXECUTABLE}"
                ${GCOVR_COMMON_ARGS}
                --fail-under-line ${QGC_COVERAGE_LINE_THRESHOLD}
                --fail-under-branch ${QGC_COVERAGE_BRANCH_THRESHOLD}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Verifying coverage thresholds (lines>=${QGC_COVERAGE_LINE_THRESHOLD}%, branches>=${QGC_COVERAGE_BRANCH_THRESHOLD}%) — run 'coverage' target first"
            VERBATIM
        )
    endif()

    add_custom_target(coverage-clean
        COMMAND "${CMAKE_COMMAND}" -E rm -f coverage.xml coverage.html
        COMMAND "${CMAKE_COMMAND}" "-DQGC_COVERAGE_BUILD_DIR=${CMAKE_BINARY_DIR}" -P
                "${CMAKE_CURRENT_LIST_DIR}/CleanCoverage.cmake"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Cleaning coverage data"
        VERBATIM
    )

else()
    message(STATUS "gcovr not found - coverage report targets not available")
    message(STATUS "  Install with: pip install gcovr")
endif()
