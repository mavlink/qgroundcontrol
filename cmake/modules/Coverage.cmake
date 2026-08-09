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
    message(WARNING "QGC: Code coverage requires Debug build, but CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
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
    # Scope coverage flags to first-party targets only. Global add_compile_options
    # would leak --coverage into CPM third-party static libraries, injecting
    # __gcov_* symbols that cause linker errors.
    # Counter updates are deliberately non-atomic: atomic updates slow hot
    # threaded paths enough to break timing-sensitive tests; the occasional
    # racy negative count is tolerated by gcovr (see negative_hits below).
    set(_QGC_COVERAGE_COMPILE_FLAGS --coverage -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage)
    set(GCOVR_GCOV_EXECUTABLE gcov)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang gcov-style coverage (llvm-cov gcov)")
    # gcov-style instrumentation (not -fprofile-instr-generate): gcovr can only
    # read gcov-format data, which "llvm-cov gcov" converts.
    set(_QGC_COVERAGE_COMPILE_FLAGS --coverage -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage)

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

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${_QGC_COVERAGE_COMPILE_FLAGS})

# First-party code also lives in separate library targets under src/ (QML
# modules such as PlanViewModule, Viewer3DModule, ...), which the main-target
# flags above do not reach — without instrumentation their sources silently
# report no coverage. Those targets do not exist yet when this module is
# included, so apply the flags in a deferred call at the end of configuration.
# Walking only src/ keeps the flags out of third-party libraries.
function(_qgc_coverage_collect_targets out_var dir)
    get_property(dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        _qgc_coverage_collect_targets(sub_targets "${subdir}")
        list(APPEND dir_targets ${sub_targets})
    endforeach()
    set(${out_var} "${dir_targets}" PARENT_SCOPE)
endfunction()

function(_qgc_coverage_instrument_src_targets)
    _qgc_coverage_collect_targets(src_targets "${CMAKE_SOURCE_DIR}/src")
    foreach(target IN LISTS src_targets)
        get_target_property(target_type ${target} TYPE)
        if(target_type MATCHES "^(STATIC_LIBRARY|OBJECT_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|EXECUTABLE)$")
            target_compile_options(${target} PRIVATE ${_QGC_COVERAGE_COMPILE_FLAGS})
        endif()
    endforeach()
endfunction()
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _qgc_coverage_instrument_src_targets)

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
        # Non-atomic profile counters race in threaded code and can go
        # negative; warn instead of aborting the report
        --gcov-ignore-parse-errors negative_hits.warn_once_per_file
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
