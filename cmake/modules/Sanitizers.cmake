# ============================================================================
# Sanitizers and Runtime Analysis for QGroundControl
# ============================================================================
#
# Unified module for runtime error detection, memory checking, and profiling.
#
# Compile-time Sanitizers (require rebuild):
#   cmake -DQGC_ENABLE_ASAN=ON ...     # AddressSanitizer (memory errors)
#   cmake -DQGC_ENABLE_UBSAN=ON ...    # UndefinedBehaviorSanitizer
#   cmake -DQGC_ENABLE_TSAN=ON ...     # ThreadSanitizer (data races)
#   cmake -DQGC_ENABLE_MSAN=ON ...     # MemorySanitizer (uninitialized reads)
#
# Runtime Analysis (no rebuild required):
#   cmake --build build --target check-memcheck      # Valgrind memcheck
#   cmake --build build --target check-helgrind      # Valgrind thread checker
#   cmake --build build --target check-callgrind     # Valgrind profiler
#
# Notes:
#   - Sanitizers require Debug or RelWithDebInfo builds
#   - ASan and TSan cannot be used together
#   - MSan requires the entire stack (including Qt) to be built with MSan
#   - ASan + UBSan can be combined
#   - Valgrind targets are always available (no rebuild needed)
#
# ============================================================================

include_guard(GLOBAL)

include(CMakeDependentOption)

# ############################################################################
# PART 1: COMPILE-TIME SANITIZERS
# ############################################################################

# Sanitizer options
#
# For multi-config generators (Visual Studio, Xcode, Ninja Multi-Config),
# CMAKE_BUILD_TYPE is typically empty at configure time. In that case, allow
# sanitizer options to be configured and validate selected configs later.
set(_qgc_sanitizers_config_supported TRUE)
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
        set(_qgc_sanitizers_config_supported FALSE)
    endif()
endif()

cmake_dependent_option(QGC_ENABLE_ASAN
    "Enable AddressSanitizer (memory error detection)"
    OFF
    "_qgc_sanitizers_config_supported"
    OFF)

cmake_dependent_option(QGC_ENABLE_UBSAN
    "Enable UndefinedBehaviorSanitizer"
    OFF
    "_qgc_sanitizers_config_supported"
    OFF)

cmake_dependent_option(QGC_ENABLE_TSAN
    "Enable ThreadSanitizer (data race detection)"
    OFF
    "_qgc_sanitizers_config_supported"
    OFF)

cmake_dependent_option(QGC_ENABLE_MSAN
    "Enable MemorySanitizer (uninitialized memory detection)"
    OFF
    "_qgc_sanitizers_config_supported"
    OFF)

if((QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN OR QGC_ENABLE_MSAN)
   AND NOT CMAKE_CONFIGURATION_TYPES
   AND NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
    message(FATAL_ERROR
        "Sanitizers require Debug or RelWithDebInfo builds for single-config generators. "
        "Current CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'.")
endif()

# Validate incompatible combinations
if(QGC_ENABLE_ASAN AND QGC_ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be used together. Choose one.")
endif()

if(QGC_ENABLE_ASAN AND QGC_ENABLE_MSAN)
    message(FATAL_ERROR "ASan and MSan cannot be used together. Choose one.")
endif()

if(QGC_ENABLE_TSAN AND QGC_ENABLE_MSAN)
    message(FATAL_ERROR "TSan and MSan cannot be used together. Choose one.")
endif()

# Check compiler support
if(QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN OR QGC_ENABLE_MSAN)
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        message(FATAL_ERROR "Sanitizers are only supported with GCC and Clang")
    endif()
endif()

# ============================================================================
# AddressSanitizer (ASan)
# ============================================================================
# Detects: buffer overflows, use-after-free, memory leaks, double-free

if(QGC_ENABLE_ASAN)
    message(STATUS "AddressSanitizer (ASan) enabled")

    set(ASAN_FLAGS -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND ASAN_FLAGS -fsanitize-address-use-after-scope)
    endif()

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${ASAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${ASAN_FLAGS})

    set(ASAN_DEFAULT_OPTIONS "detect_leaks=1:halt_on_error=0:print_stats=1:check_initialization_order=1")

    file(WRITE ${CMAKE_BINARY_DIR}/run-with-asan.sh
"#!/bin/bash
export ASAN_OPTIONS=\"${ASAN_DEFAULT_OPTIONS}\"
export ASAN_SYMBOLIZER_PATH=\"$(which llvm-symbolizer 2>/dev/null || which addr2line)\"
exec \"$@\"
")
    file(CHMOD ${CMAKE_BINARY_DIR}/run-with-asan.sh PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    message(STATUS "  Run with: ASAN_OPTIONS=\"${ASAN_DEFAULT_OPTIONS}\" ./QGroundControl")
endif()

# ============================================================================
# UndefinedBehaviorSanitizer (UBSan)
# ============================================================================
# Detects: integer overflow, null dereference, division by zero, invalid shifts

if(QGC_ENABLE_UBSAN)
    message(STATUS "UndefinedBehaviorSanitizer (UBSan) enabled")

    # GCC does not support Clang's grouped UBSan checks ("integer", "nullability").
    # Keep a common baseline and extend checks only where the compiler supports them.
    set(UBSAN_CHECKS "undefined")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND UBSAN_CHECKS "integer" "nullability")
    endif()
    set(UBSAN_EXCLUDES "-fno-sanitize=vptr")  # Qt triggers this

    string(REPLACE ";" "," UBSAN_CHECKS_STR "${UBSAN_CHECKS}")
    set(UBSAN_FLAGS -fsanitize=${UBSAN_CHECKS_STR} ${UBSAN_EXCLUDES} -fno-omit-frame-pointer)

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${UBSAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${UBSAN_FLAGS})

    set(UBSAN_DEFAULT_OPTIONS "print_stacktrace=1:halt_on_error=0")
    message(STATUS "  Run with: UBSAN_OPTIONS=\"${UBSAN_DEFAULT_OPTIONS}\" ./QGroundControl")
endif()

# ============================================================================
# ThreadSanitizer (TSan)
# ============================================================================
# Detects: data races, deadlocks, lock order inversions

if(QGC_ENABLE_TSAN)
    message(STATUS "ThreadSanitizer (TSan) enabled")

    set(TSAN_FLAGS -fsanitize=thread -fno-omit-frame-pointer)

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${TSAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${TSAN_FLAGS})

    set(TSAN_DEFAULT_OPTIONS "second_deadlock_stack=1:halt_on_error=0")
    message(STATUS "  Run with: TSAN_OPTIONS=\"${TSAN_DEFAULT_OPTIONS}\" ./QGroundControl")
    message(STATUS "  Note: You may need to increase stack size: ulimit -s unlimited")
endif()

# ============================================================================
# MemorySanitizer (MSan) - Clang only
# ============================================================================
# Detects: uninitialized memory reads
# Note: Requires ALL libraries (including Qt) to be MSan-instrumented

if(QGC_ENABLE_MSAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "MemorySanitizer is only supported with Clang")
    endif()

    message(STATUS "MemorySanitizer (MSan) enabled")
    message(WARNING "MSan requires ALL dependencies (including Qt) to be MSan-instrumented!")

    set(MSAN_FLAGS -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins=2)

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${MSAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${MSAN_FLAGS})

    set(MSAN_DEFAULT_OPTIONS "halt_on_error=0")
    message(STATUS "  Run with: MSAN_OPTIONS=\"${MSAN_DEFAULT_OPTIONS}\" ./QGroundControl")
endif()

# ============================================================================
# Sanitizer Suppression Files
# ============================================================================

if(QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN)
    file(WRITE ${CMAKE_BINARY_DIR}/asan_suppressions.txt
"# QGC ASan Suppressions
leak:libQt
leak:qt_
leak:libfontconfig
leak:libpulse
leak:QGCCorePlugin::QGCCorePlugin
leak:QGCOptions::QGCOptions
")

    file(WRITE ${CMAKE_BINARY_DIR}/tsan_suppressions.txt
"# QGC TSan Suppressions
race:QObject::
race:QMetaObject::
race:std::__1::
")

    file(WRITE ${CMAKE_BINARY_DIR}/ubsan_suppressions.txt
"# QGC UBSan Suppressions
vptr:libQt
")

    message(STATUS "Sanitizer suppression files created in ${CMAKE_BINARY_DIR}")
endif()

# ============================================================================
# Sanitizer Test Targets (require QGC_BUILD_TESTING for CTest)
# ============================================================================

if(QGC_BUILD_TESTING)
    if(QGC_ENABLE_ASAN)
        add_custom_target(check-asan
            COMMAND ${CMAKE_COMMAND} -E env
                "ASAN_OPTIONS=${ASAN_DEFAULT_OPTIONS}"
                "LSAN_OPTIONS=suppressions=${CMAKE_BINARY_DIR}/asan_suppressions.txt"
                ${CMAKE_CTEST_COMMAND} --output-on-failure -L Unit
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running unit tests with AddressSanitizer"
            VERBATIM
        )
        add_dependencies(check-asan ${CMAKE_PROJECT_NAME})
    endif()

    if(QGC_ENABLE_TSAN)
        add_custom_target(check-tsan
            COMMAND ${CMAKE_COMMAND} -E env
                "TSAN_OPTIONS=${TSAN_DEFAULT_OPTIONS}:suppressions=${CMAKE_BINARY_DIR}/tsan_suppressions.txt"
                ${CMAKE_CTEST_COMMAND} --output-on-failure -L Unit
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running unit tests with ThreadSanitizer"
            VERBATIM
        )
        add_dependencies(check-tsan ${CMAKE_PROJECT_NAME})
    endif()
endif()

# ############################################################################
# PART 2: VALGRIND RUNTIME ANALYSIS
# ############################################################################
# No rebuild required - runs existing binary under Valgrind

# ----------------------------------------------------------------------------
# Find Valgrind
# ----------------------------------------------------------------------------
find_program(VALGRIND_EXECUTABLE valgrind
    HINTS /usr/bin /usr/local/bin
    DOC "Path to valgrind executable"
)

if(VALGRIND_EXECUTABLE)
    execute_process(
        COMMAND ${VALGRIND_EXECUTABLE} --version
        OUTPUT_VARIABLE VALGRIND_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    message(STATUS "Found Valgrind: ${VALGRIND_EXECUTABLE} (${VALGRIND_VERSION})")

    # Suppression file
    set(QGC_VALGRIND_SUPP "${CMAKE_SOURCE_DIR}/tools/debuggers/valgrind.supp")
    if(EXISTS ${QGC_VALGRIND_SUPP})
        message(STATUS "Using Valgrind suppressions: ${QGC_VALGRIND_SUPP}")
    endif()

    # CTest memcheck configuration and targets (require QGC_BUILD_TESTING)
    if(QGC_BUILD_TESTING)
        set(MEMORYCHECK_COMMAND ${VALGRIND_EXECUTABLE})
        set(MEMORYCHECK_TYPE Valgrind)
        set(MEMORYCHECK_SUPPRESSIONS_FILE ${QGC_VALGRIND_SUPP})
        set(MEMORYCHECK_COMMAND_OPTIONS
            "--tool=memcheck"
            "--leak-check=full"
            "--show-leak-kinds=definite,possible"
            "--track-origins=yes"
            "--trace-children=yes"
            "--error-exitcode=1"
            "--gen-suppressions=all"
            "--num-callers=50"
            CACHE STRING "Valgrind command options"
        )

        if(EXISTS "${CMAKE_SOURCE_DIR}/CTestCustom.cmake.in")
            configure_file(
                "${CMAKE_SOURCE_DIR}/CTestCustom.cmake.in"
                "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
                @ONLY
            )
        endif()

        math(EXPR QGC_VALGRIND_TIMEOUT "${QGC_VALGRIND_TIMEOUT_MULTIPLIER} * 100")

        # ====================================================================
        # Valgrind Memcheck Targets
        # ====================================================================

        add_custom_target(check-memcheck
            COMMAND ${CMAKE_CTEST_COMMAND}
                -T memcheck
                --output-on-failure
                --timeout ${QGC_VALGRIND_TIMEOUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running tests under Valgrind memcheck"
            VERBATIM
        )
        add_dependencies(check-memcheck ${CMAKE_PROJECT_NAME})

        add_custom_target(check-memcheck-unit
            COMMAND ${CMAKE_CTEST_COMMAND}
                -T memcheck
                -L Unit
                --output-on-failure
                --timeout ${QGC_VALGRIND_TIMEOUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running unit tests under Valgrind memcheck"
            VERBATIM
        )
        add_dependencies(check-memcheck-unit ${CMAKE_PROJECT_NAME})

        add_custom_target(check-memcheck-quick
            COMMAND ${CMAKE_CTEST_COMMAND}
                -T memcheck
                -LE "Slow|Integration"
                --output-on-failure
                --timeout ${QGC_VALGRIND_TIMEOUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running quick tests under Valgrind memcheck"
            VERBATIM
        )
        add_dependencies(check-memcheck-quick ${CMAKE_PROJECT_NAME})
    endif()

    # ========================================================================
    # Direct Valgrind Invocation
    # ========================================================================

    add_custom_target(valgrind-app
        COMMAND ${VALGRIND_EXECUTABLE}
            --tool=memcheck
            --leak-check=full
            --show-leak-kinds=definite,possible
            --track-origins=yes
            --suppressions=${QGC_VALGRIND_SUPP}
            --log-file=valgrind-app.log
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
        COMMENT "Running application under Valgrind (output: valgrind-app.log)"
        VERBATIM
    )
    add_dependencies(valgrind-app ${CMAKE_PROJECT_NAME})

    # Test-dependent Valgrind targets (require --unittest support)
    if(QGC_BUILD_TESTING)
        add_custom_target(valgrind-test
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=memcheck
                --leak-check=full
                --show-leak-kinds=definite,possible
                --track-origins=yes
                --error-exitcode=1
                --suppressions=${QGC_VALGRIND_SUPP}
                --log-file=valgrind-test.log
                $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running tests under Valgrind (output: valgrind-test.log)"
            VERBATIM
        )
        add_dependencies(valgrind-test ${CMAKE_PROJECT_NAME})

        # ====================================================================
        # Helgrind (Thread Error Detection)
        # ====================================================================

        add_custom_target(check-helgrind
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=helgrind
                --history-level=full
                --suppressions=${QGC_VALGRIND_SUPP}
                --log-file=helgrind.log
                $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running thread error detection with Helgrind (output: helgrind.log)"
            VERBATIM
        )
        add_dependencies(check-helgrind ${CMAKE_PROJECT_NAME})

        # ====================================================================
        # Cachegrind (Cache Profiling)
        # ====================================================================

        add_custom_target(check-cachegrind
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=cachegrind
                --cachegrind-out-file=cachegrind.out
                $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest
            COMMAND ${CMAKE_COMMAND} -E echo "Run 'cg_annotate cachegrind.out' to view results"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running cache profiling with Cachegrind"
            VERBATIM
        )
        add_dependencies(check-cachegrind ${CMAKE_PROJECT_NAME})

        # ====================================================================
        # Callgrind (Call Graph Profiling)
        # ====================================================================

        add_custom_target(check-callgrind
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=callgrind
                --callgrind-out-file=callgrind.out
                $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest
            COMMAND ${CMAKE_COMMAND} -E echo "Run 'kcachegrind callgrind.out' to view results"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
            COMMENT "Running call graph profiling with Callgrind"
            VERBATIM
        )
        add_dependencies(check-callgrind ${CMAKE_PROJECT_NAME})
    endif()

else()
    message(STATUS "Valgrind not found - runtime analysis targets not available")
endif()
