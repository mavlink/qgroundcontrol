# ============================================================================
# QGroundControl Test Infrastructure
# ============================================================================
# CTest integration with labels, timeouts, and resource locking.
#
# Usage:
#   include(QGCTest)
#   add_qgc_test(MyTest LABELS Unit)
#   add_qgc_test(MyIntegrationTest LABELS Integration RESOURCE_LOCK MockLink)

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------

cmake_host_system_information(RESULT _num_cores QUERY NUMBER_OF_LOGICAL_CORES)
set(QGC_TEST_PARALLEL_LEVEL ${_num_cores} CACHE STRING "Number of parallel test jobs")

set(QGC_TEST_TIMEOUT_UNIT 60 CACHE STRING "Timeout for unit tests (seconds)")
set(QGC_TEST_TIMEOUT_INTEGRATION 120 CACHE STRING "Timeout for integration tests (seconds)")
set(QGC_TEST_TIMEOUT_SLOW 180 CACHE STRING "Timeout for slow tests (seconds)")
set(QGC_TEST_TIMEOUT_DEFAULT 90 CACHE STRING "Default test timeout (seconds)")

# ----------------------------------------------------------------------------
# Convenience Targets
# ----------------------------------------------------------------------------

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running all tests"
)

add_custom_target(check-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -L Unit --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running unit tests"
)

add_custom_target(check-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L Integration --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running integration tests"
)

add_custom_target(check-fast
    COMMAND ${CMAKE_CTEST_COMMAND} -LE Slow --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running fast tests (excluding Slow)"
)

add_custom_target(check-ci
    COMMAND ${CMAKE_CTEST_COMMAND} -LE "Flaky|Network" --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running CI-safe tests"
    VERBATIM
)

# Category-specific targets
foreach(_category MissionManager Vehicle Utilities MAVLink Comms)
    string(TOLOWER ${_category} _target_suffix)
    add_custom_target(check-${_target_suffix}
        COMMAND ${CMAKE_CTEST_COMMAND} -L ${_category} --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
        USES_TERMINAL
        COMMENT "Running ${_category} tests"
    )
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
#   SERIAL              - Shorthand for locking all shared resources
#
# Labels:
#   Unit        - Fast, isolated tests (~30s timeout)
#   Integration - Tests requiring MockLink/Vehicle (~60s timeout)
#   Slow        - Long-running tests (~120s timeout)
#   Flaky       - Tests with intermittent failures (excluded from CI)
#   Network     - Tests requiring network access (excluded from CI)
#
# Example:
#   add_qgc_test(ParameterManagerTest LABELS Integration Vehicle SERIAL)

function(add_qgc_test test_name)
    cmake_parse_arguments(ARG "SERIAL" "TIMEOUT" "LABELS;RESOURCE_LOCK" ${ARGN})

    add_test(
        NAME ${test_name}
        COMMAND $<TARGET_FILE:${PROJECT_NAME}> --unittest:${test_name} --allow-multiple
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Determine timeout based on labels or explicit value
    if(ARG_TIMEOUT)
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

    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT ${_timeout}
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false"
        FAIL_REGULAR_EXPRESSION "FAIL!;Segmentation fault;ASSERT"
    )

    if(ARG_LABELS)
        set_tests_properties(${test_name} PROPERTIES LABELS "${ARG_LABELS}")
    endif()

    # Resource locking for tests that can't run in parallel
    if(ARG_SERIAL)
        set_tests_properties(${test_name} PROPERTIES
            RESOURCE_LOCK "MockLink;Vehicle;ParameterManager;MissionController"
            RUN_SERIAL TRUE
        )
    elseif(ARG_RESOURCE_LOCK)
        set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK "${ARG_RESOURCE_LOCK}")
    elseif("Integration" IN_LIST ARG_LABELS)
        set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK "MockLink")
    endif()

    # Add dependency so 'check' target builds first
    add_dependencies(check ${PROJECT_NAME})
endfunction()
