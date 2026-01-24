# ============================================================================
# Sanitizer Configuration for QGroundControl
# ============================================================================
#
# Enables runtime sanitizers for detecting memory errors, undefined behavior,
# and thread issues during testing.
#
# Usage:
#   cmake -DQGC_ENABLE_ASAN=ON ...     # AddressSanitizer (memory errors)
#   cmake -DQGC_ENABLE_UBSAN=ON ...    # UndefinedBehaviorSanitizer
#   cmake -DQGC_ENABLE_TSAN=ON ...     # ThreadSanitizer (data races)
#   cmake -DQGC_ENABLE_MSAN=ON ...     # MemorySanitizer (uninitialized reads)
#
# Notes:
#   - Sanitizers require Debug or RelWithDebInfo builds
#   - ASan and TSan cannot be used together
#   - MSan requires the entire stack (including Qt) to be built with MSan
#   - ASan + UBSan can be combined
#
# Environment variables for runtime:
#   ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
#   UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
#   TSAN_OPTIONS=second_deadlock_stack=1
#
# ============================================================================

include(CMakeDependentOption)

# Sanitizer options (only available in Debug/RelWithDebInfo)
cmake_dependent_option(QGC_ENABLE_ASAN
    "Enable AddressSanitizer (memory error detection)"
    OFF
    "CMAKE_BUILD_TYPE MATCHES Debug|RelWithDebInfo"
    OFF)

cmake_dependent_option(QGC_ENABLE_UBSAN
    "Enable UndefinedBehaviorSanitizer"
    OFF
    "CMAKE_BUILD_TYPE MATCHES Debug|RelWithDebInfo"
    OFF)

cmake_dependent_option(QGC_ENABLE_TSAN
    "Enable ThreadSanitizer (data race detection)"
    OFF
    "CMAKE_BUILD_TYPE MATCHES Debug|RelWithDebInfo"
    OFF)

cmake_dependent_option(QGC_ENABLE_MSAN
    "Enable MemorySanitizer (uninitialized memory detection)"
    OFF
    "CMAKE_BUILD_TYPE MATCHES Debug|RelWithDebInfo"
    OFF)

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
        message(WARNING "Sanitizers are only supported with GCC and Clang")
        return()
    endif()
endif()

# ============================================================================
# AddressSanitizer (ASan)
# ============================================================================
# Detects:
#   - Buffer overflows (stack, heap, global)
#   - Use-after-free
#   - Use-after-return
#   - Memory leaks (with detect_leaks=1)
#   - Double-free

if(QGC_ENABLE_ASAN)
    message(STATUS "AddressSanitizer (ASan) enabled")

    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")

    # Additional options for better detection
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(ASAN_FLAGS "${ASAN_FLAGS} -fsanitize-address-use-after-scope")
    endif()

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${ASAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${ASAN_FLAGS})

    # Set default runtime options
    set(ASAN_DEFAULT_OPTIONS "detect_leaks=1:halt_on_error=0:print_stats=1:check_initialization_order=1")

    # Create a helper script for running with ASan
    file(WRITE ${CMAKE_BINARY_DIR}/run-with-asan.sh
"#!/bin/bash
export ASAN_OPTIONS=\"${ASAN_DEFAULT_OPTIONS}\"
export ASAN_SYMBOLIZER_PATH=\"$(which llvm-symbolizer 2>/dev/null || which addr2line)\"
exec \"$@\"
")
    execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/run-with-asan.sh)

    message(STATUS "  Run with: ASAN_OPTIONS=\"${ASAN_DEFAULT_OPTIONS}\" ./QGroundControl")
endif()

# ============================================================================
# UndefinedBehaviorSanitizer (UBSan)
# ============================================================================
# Detects:
#   - Signed integer overflow
#   - Null pointer dereference
#   - Division by zero
#   - Invalid shifts
#   - Out-of-bounds array access
#   - Invalid type casts

if(QGC_ENABLE_UBSAN)
    message(STATUS "UndefinedBehaviorSanitizer (UBSan) enabled")

    # Select specific checks (some are noisy with Qt)
    set(UBSAN_CHECKS
        "undefined"
        "integer"
        "nullability"
    )

    # Exclude some checks that Qt triggers
    set(UBSAN_EXCLUDES
        "-fno-sanitize=vptr"  # Qt uses RTTI in ways that trigger this
    )

    string(REPLACE ";" "," UBSAN_CHECKS_STR "${UBSAN_CHECKS}")
    set(UBSAN_FLAGS "-fsanitize=${UBSAN_CHECKS_STR} ${UBSAN_EXCLUDES} -fno-omit-frame-pointer")

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${UBSAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${UBSAN_FLAGS})

    set(UBSAN_DEFAULT_OPTIONS "print_stacktrace=1:halt_on_error=0")
    message(STATUS "  Run with: UBSAN_OPTIONS=\"${UBSAN_DEFAULT_OPTIONS}\" ./QGroundControl")
endif()

# ============================================================================
# ThreadSanitizer (TSan)
# ============================================================================
# Detects:
#   - Data races
#   - Deadlocks
#   - Lock order inversions

if(QGC_ENABLE_TSAN)
    message(STATUS "ThreadSanitizer (TSan) enabled")

    set(TSAN_FLAGS "-fsanitize=thread -fno-omit-frame-pointer")

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${TSAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${TSAN_FLAGS})

    set(TSAN_DEFAULT_OPTIONS "second_deadlock_stack=1:halt_on_error=0")
    message(STATUS "  Run with: TSAN_OPTIONS=\"${TSAN_DEFAULT_OPTIONS}\" ./QGroundControl")

    # TSan requires more stack space
    message(STATUS "  Note: You may need to increase stack size: ulimit -s unlimited")
endif()

# ============================================================================
# MemorySanitizer (MSan) - Clang only
# ============================================================================
# Detects:
#   - Uninitialized memory reads
#
# Note: MSan requires ALL libraries (including Qt) to be built with MSan
# instrumentation, which is impractical for most users. This is mainly
# useful for CI environments with specially-built dependencies.

if(QGC_ENABLE_MSAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "MemorySanitizer is only supported with Clang")
    endif()

    message(STATUS "MemorySanitizer (MSan) enabled")
    message(WARNING "MSan requires ALL dependencies (including Qt) to be MSan-instrumented!")

    set(MSAN_FLAGS "-fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins=2")

    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${MSAN_FLAGS})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${MSAN_FLAGS})

    set(MSAN_DEFAULT_OPTIONS "halt_on_error=0")
    message(STATUS "  Run with: MSAN_OPTIONS=\"${MSAN_DEFAULT_OPTIONS}\" ./QGroundControl")
endif()

# ============================================================================
# Sanitizer Suppression Files
# ============================================================================
# Create suppression files for known false positives

if(QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN)
    # ASan suppressions (leak detection)
    file(WRITE ${CMAKE_BINARY_DIR}/asan_suppressions.txt
"# QGC ASan Suppressions
# Add patterns for known false positives or third-party leaks

# Qt internal allocations (if any)
leak:libQt
leak:qt_

# System libraries
leak:libfontconfig
leak:libpulse
")

    # TSan suppressions
    file(WRITE ${CMAKE_BINARY_DIR}/tsan_suppressions.txt
"# QGC TSan Suppressions
# Add patterns for known benign races

# Qt internals
race:QObject::
race:QMetaObject::

# Standard library
race:std::__1::
")

    # UBSan suppressions
    file(WRITE ${CMAKE_BINARY_DIR}/ubsan_suppressions.txt
"# QGC UBSan Suppressions
# Suppress known issues in third-party code

# Qt uses some patterns that trigger UBSan
vptr:libQt
")

    message(STATUS "Sanitizer suppression files created in ${CMAKE_BINARY_DIR}")
endif()

# ============================================================================
# CTest Integration
# ============================================================================
# Add test targets that run with sanitizers

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
