# ============================================================================
# QGroundControl Test Infrastructure
# ============================================================================
# CTest integration with labels, timeouts, and resource locking.
#
# Usage:
#   include(QGCTest)
#   add_qgc_test(MyTest LABELS Unit)
#   add_qgc_test(MyIntegrationTest LABELS Integration RESOURCE_LOCK MockLink)

include_guard(GLOBAL)

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------

cmake_host_system_information(RESULT _num_cores QUERY NUMBER_OF_LOGICAL_CORES)
if(NOT _num_cores MATCHES "^[1-9][0-9]*$")
    set(_num_cores 1)
endif()
set(QGC_TEST_PARALLEL_LEVEL
    ${_num_cores}
    CACHE STRING "Number of parallel test jobs"
)

set(QGC_TEST_TIMEOUT_UNIT
    60
    CACHE STRING "Timeout for unit tests (seconds)"
)
set(QGC_TEST_TIMEOUT_INTEGRATION
    120
    CACHE STRING "Timeout for integration tests (seconds)"
)
set(QGC_TEST_TIMEOUT_SLOW
    180
    CACHE STRING "Timeout for slow tests (seconds)"
)
set(QGC_TEST_TIMEOUT_DEFAULT
    90
    CACHE STRING "Default test timeout (seconds)"
)

option(QGC_TEST_ONSCREEN "Run tests with native display instead of offscreen" OFF)

# LSan's tracer process needs ptrace, which Yama (ptrace_scope>=1) and containers
# often block, failing tests with "Tracer caught signal 11" at exit. Tests therefore
# run with detect_leaks=0 unless this is ON, as in the Linux-sanitizers preset.
option(QGC_TEST_DETECT_LEAKS "Allow LSan leak detection under ASan (requires ptrace permission)" OFF)

foreach(_qgc_positive_setting IN ITEMS QGC_TEST_PARALLEL_LEVEL)
    if(NOT "${${_qgc_positive_setting}}" MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "${_qgc_positive_setting} must be a positive integer")
    endif()
endforeach()

foreach(
    _qgc_timeout_setting IN ITEMS QGC_TEST_TIMEOUT_UNIT QGC_TEST_TIMEOUT_INTEGRATION QGC_TEST_TIMEOUT_SLOW
                                  QGC_TEST_TIMEOUT_DEFAULT
)
    if(NOT "${${_qgc_timeout_setting}}" MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${_qgc_timeout_setting} must be a non-negative integer")
    endif()
endforeach()

# ----------------------------------------------------------------------------
# Convenience Targets
# ----------------------------------------------------------------------------

add_custom_target(
    check
    COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" --no-tests=error --output-on-failure --parallel
            ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running all tests"
    VERBATIM
)

add_custom_target(
    check-unit
    COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" -L Unit --no-tests=error --output-on-failure --parallel
            ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running unit tests"
    VERBATIM
)

add_custom_target(
    check-integration
    COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" -L Integration --no-tests=error --output-on-failure
            --parallel 1
    USES_TERMINAL
    COMMENT "Running integration tests"
    VERBATIM
)

add_custom_target(
    check-fast
    COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" -LE Slow --no-tests=error --output-on-failure --parallel
            ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running fast tests (excluding Slow)"
    VERBATIM
)

add_custom_target(
    check-ci
    COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" -LE "Flaky|Network" --no-tests=error --output-on-failure
            --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running CI-safe tests"
    VERBATIM
)

set(QGC_TEST_FLAKY_REPEAT
    3
    CACHE STRING "Repeat count for the check-flaky target"
)
if(NOT QGC_TEST_FLAKY_REPEAT MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "QGC_TEST_FLAKY_REPEAT must be a positive integer")
endif()
add_custom_target(
    check-flaky
    COMMAND
        "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" -LE "Network" --repeat until-fail:${QGC_TEST_FLAKY_REPEAT}
        --no-tests=error --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running tests ${QGC_TEST_FLAKY_REPEAT}x (until-fail) to surface flaky failures"
    VERBATIM
)

# Category-specific targets
set(_qgc_test_categories MissionManager Vehicle Utilities MAVLink Comms)
foreach(_category IN LISTS _qgc_test_categories)
    string(TOLOWER ${_category} _target_suffix)
    add_custom_target(
        check-${_target_suffix}
        COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "$<CONFIG>" -L ${_category} --no-tests=error --output-on-failure
                --parallel ${QGC_TEST_PARALLEL_LEVEL}
        USES_TERMINAL
        COMMENT "Running ${_category} tests"
        VERBATIM
    )
endforeach()

# Collect all check targets for build dependency injection
set(_qgc_check_targets check check-unit check-integration check-fast check-ci check-flaky)
foreach(_category IN LISTS _qgc_test_categories)
    string(TOLOWER ${_category} _target_suffix)
    list(APPEND _qgc_check_targets check-${_target_suffix})
endforeach()

# Ensure all check targets build the main executable first
foreach(_target IN LISTS _qgc_check_targets)
    add_dependencies(${_target} ${CMAKE_PROJECT_NAME})
endforeach()

# ----------------------------------------------------------------------------
# add_qgc_test()
# ----------------------------------------------------------------------------
# Registers a test with CTest and configures properties.
#
# Arguments:
#   test_name           - Name of the test (must match UnitTestList registration)
#   LABELS label...     - Test labels for filtering (Unit, Integration, Slow, etc.)
#   TIMEOUT seconds     - Override default timeout
#   RESOURCE_LOCK res.. - Resources that prevent parallel execution
#   SERIAL              - Run this test alone (CTest RUN_SERIAL) in sanitizer and coverage builds, whose CPU
#                         starvation delays these timing-sensitive tests past their timeouts; elsewhere it
#                         reserves two CTest processors instead
#   SHARDS count        - Split the test functions across count CTest entries
#                         (<test_name>.1 .. <test_name>.<count>) that can run in parallel
#
# Labels:
#   Unit        - Fast, isolated tests (60s timeout)
#   Integration - Tests requiring multiple components (120s timeout)
#   Slow        - Long-running tests (180s timeout)
#   Flaky       - Tests with intermittent failures (excluded from CI)
#   Network     - Tests requiring network access (excluded from CI)
#   NoSanitizer - Tests incompatible with ASan/UBSan (excluded from sanitizer CI)
#
# Example:
#   add_qgc_test(ParameterManagerTest LABELS Integration Vehicle SERIAL)
function(add_qgc_test test_name)
    # Argument validation and per-shard registration keep this public entry point long.
    # cmake-lint: disable=R0912,R0915
    cmake_parse_arguments(PARSE_ARGV 1 ARG "SERIAL" "TIMEOUT;SHARDS" "LABELS;RESOURCE_LOCK;SKIP_REGEX;ENV_MODIFICATION")

    if(NOT test_name)
        message(FATAL_ERROR "add_qgc_test: test name is required")
    endif()
    if(NOT test_name MATCHES "^[A-Za-z0-9_.+-]+$")
        message(FATAL_ERROR "add_qgc_test(${test_name}): test name contains unsafe characters")
    endif()
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "add_qgc_test(${test_name}): missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "add_qgc_test(${test_name}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(ARG_SERIAL AND ARG_RESOURCE_LOCK)
        message(FATAL_ERROR "add_qgc_test(${test_name}): SERIAL and RESOURCE_LOCK are mutually exclusive")
    endif()
    if(DEFINED ARG_TIMEOUT AND NOT ARG_TIMEOUT MATCHES "^[0-9]+$")
        message(FATAL_ERROR "add_qgc_test(${test_name}): TIMEOUT must be a non-negative integer")
    endif()
    if(DEFINED ARG_SHARDS AND (NOT ARG_SHARDS MATCHES "^[0-9]+$" OR ARG_SHARDS LESS 2))
        message(FATAL_ERROR "add_qgc_test(${test_name}): SHARDS must be an integer greater than 1")
    endif()

    set(_test_command $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest:${test_name} --allow-multiple)
    if(QGC_TEST_ONSCREEN)
        list(APPEND _test_command --onscreen)
    endif()

    if(DEFINED ARG_TIMEOUT)
        set(_timeout ${ARG_TIMEOUT})
    elseif("Slow" IN_LIST ARG_LABELS)
        set(_timeout ${QGC_TEST_TIMEOUT_SLOW})
    elseif("Integration" IN_LIST ARG_LABELS)
        set(_timeout ${QGC_TEST_TIMEOUT_INTEGRATION})
    elseif("Unit" IN_LIST ARG_LABELS)
        set(_timeout ${QGC_TEST_TIMEOUT_UNIT})
    else()
        set(_timeout ${QGC_TEST_TIMEOUT_DEFAULT})
    endif()

    set(_base_test_env "QT_LOGGING_RULES=*.debug=false")
    if(NOT QGC_TEST_ONSCREEN)
        # Offscreen plugin + software Quick backend; LIBGL_ALWAYS_SOFTWARE forces the
        # GLX path (under xvfb, see cmake_helper.py) onto Mesa llvmpipe, no GPU needed.
        list(PREPEND _base_test_env "QT_QPA_PLATFORM=offscreen" "QT_QUICK_BACKEND=software" "LIBGL_ALWAYS_SOFTWARE=1")
    endif()

    # Shard index 0 registers the unsharded test
    set(_shard_indices 0)
    if(DEFINED ARG_SHARDS)
        set(_shard_indices "")
        foreach(_shard RANGE 1 ${ARG_SHARDS} 1)
            list(APPEND _shard_indices ${_shard})
        endforeach()
    endif()

    foreach(_shard IN LISTS _shard_indices)
        if(_shard)
            set(_ctest_name "${test_name}.${_shard}")
            set(_ctest_command ${_test_command} --unittest-shard:${_shard}/${ARG_SHARDS})
        else()
            set(_ctest_name "${test_name}")
            set(_ctest_command ${_test_command})
        endif()

        add_test(
            NAME ${_ctest_name}
            COMMAND ${_ctest_command}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )

        # Per-test isolated temp dir so tests writing under QDir::tempPath() /
        # QStandardPaths::TempLocation (both honor TMPDIR on Unix, TMP/TEMP on
        # Windows) never collide. This replaces the shared TempFiles RESOURCE_LOCK
        # and needs no C++ change — QTemporaryDir/QStandardPaths pick it up.
        set(_test_env ${_base_test_env})
        set(_test_tmpdir "${CMAKE_BINARY_DIR}/test-tmp/${_ctest_name}")
        file(MAKE_DIRECTORY "${_test_tmpdir}")
        list(APPEND _test_env "TMPDIR=${_test_tmpdir}")
        if(WIN32)
            list(APPEND _test_env "TMP=${_test_tmpdir}" "TEMP=${_test_tmpdir}")
        endif()

        set_tests_properties(
            ${_ctest_name}
            PROPERTIES
                TIMEOUT ${_timeout}
                ENVIRONMENT "${_test_env}"
                FAIL_REGULAR_EXPRESSION "FAIL!;Segmentation fault"
        )

        if(ARG_LABELS)
            set_tests_properties(${_ctest_name} PROPERTIES LABELS "${ARG_LABELS}")
        endif()

        if(ARG_SKIP_REGEX)
            set_tests_properties(${_ctest_name} PROPERTIES SKIP_REGULAR_EXPRESSION "${ARG_SKIP_REGEX}")
        endif()

        if(ARG_ENV_MODIFICATION)
            set_tests_properties(${_ctest_name} PROPERTIES ENVIRONMENT_MODIFICATION "${ARG_ENV_MODIFICATION}")
        endif()

        # Resource locking for tests that can't run in parallel
        if(ARG_SERIAL)
            if(QGC_ENABLE_ASAN
               OR QGC_ENABLE_UBSAN
               OR QGC_ENABLE_TSAN
               OR QGC_ENABLE_MSAN
               OR QGC_ENABLE_COVERAGE
            )
                set_tests_properties(${_ctest_name} PROPERTIES RUN_SERIAL TRUE)
            else()
                set_tests_properties(${_ctest_name} PROPERTIES PROCESSORS 2)
            endif()
        elseif(ARG_RESOURCE_LOCK)
            # TempFiles is now handled by per-test isolated TMPDIR above; strip it so
            # those tests parallelize. Genuinely serial locks (MockLink, Joystick,
            # Settings, fixed ports, PlanViewSettings) are preserved.
            set(_resource_lock ${ARG_RESOURCE_LOCK})
            list(REMOVE_ITEM _resource_lock TempFiles)
            if(_resource_lock)
                set_tests_properties(${_ctest_name} PROPERTIES RESOURCE_LOCK "${_resource_lock}")
            endif()
        endif()
    endforeach()

endfunction()

# ----------------------------------------------------------------------------
# Sanitizer Runtime Options
# ----------------------------------------------------------------------------
# Every test halts on its first sanitizer report so findings fail it, as in CI. These replace any
# *SAN_OPTIONS in the caller's environment; run a test executable directly to try other options.

# Appends test_environment to the ENVIRONMENT of every test in directory and its subdirectories.
function(_qgc_append_test_environment directory test_environment)
    get_property(
        _tests
        DIRECTORY "${directory}"
        PROPERTY TESTS
    )
    if(_tests)
        set_property(
            TEST ${_tests}
            DIRECTORY "${directory}"
            APPEND
            PROPERTY ENVIRONMENT "${test_environment}"
        )
    endif()
    get_property(
        _subdirectories
        DIRECTORY "${directory}"
        PROPERTY SUBDIRECTORIES
    )
    foreach(_subdirectory IN LISTS _subdirectories)
        _qgc_append_test_environment("${_subdirectory}" "${test_environment}")
    endforeach()
endfunction()

# Sets the enabled sanitizers' runtime options for every test in the tree.
function(_qgc_apply_sanitizer_test_environment)
    set(_dir "${CMAKE_BINARY_DIR}")
    set(_options)
    if(QGC_ENABLE_ASAN)
        if(QGC_TEST_DETECT_LEAKS)
            set(_detect_leaks 1)
        else()
            set(_detect_leaks 0)
        endif()
        list(APPEND _options "ASAN_OPTIONS=detect_leaks=${_detect_leaks}:halt_on_error=1:check_initialization_order=1"
             "LSAN_OPTIONS=suppressions=${_dir}/asan_suppressions.txt"
        )
    endif()
    if(QGC_ENABLE_UBSAN)
        list(APPEND _options
             "UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1:suppressions=${_dir}/ubsan_suppressions.txt"
        )
    endif()
    if(QGC_ENABLE_TSAN)
        list(APPEND _options
             "TSAN_OPTIONS=second_deadlock_stack=1:halt_on_error=1:suppressions=${_dir}/tsan_suppressions.txt"
        )
    endif()
    if(QGC_ENABLE_MSAN)
        list(APPEND _options "MSAN_OPTIONS=halt_on_error=1")
    endif()
    if(_options)
        _qgc_append_test_environment("${CMAKE_CURRENT_SOURCE_DIR}" "${_options}")
    endif()
endfunction()

# Runs once the whole tree is configured, so tests registered with add_test(), such as the standalone
# library and portable tests, get the same options.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _qgc_apply_sanitizer_test_environment)
