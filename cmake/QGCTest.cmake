# ============================================================================
# QGroundControl Test Configuration
# ============================================================================
# CTest integration for QGroundControl unit tests.
# See test/TESTING.md for full documentation.
#
# Usage:
#   include(QGCTest)
#   add_qgc_test(MyTest LABELS Unit Category)
#   add_qgc_test(MyTest LABELS Integration RESOURCE_LOCK MockLink)

if(NOT QGC_BUILD_TESTING)
    return()
endif()

cmake_host_system_information(RESULT QGC_TEST_PARALLEL_LEVEL QUERY NUMBER_OF_LOGICAL_CORES)

# ----------------------------------------------------------------------------
# Convenience Targets
# ----------------------------------------------------------------------------

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running all tests"
)
add_dependencies(check ${PROJECT_NAME})

add_custom_target(check-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -L Unit --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running unit tests"
)
add_dependencies(check-unit ${PROJECT_NAME})

add_custom_target(check-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L Integration --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running integration tests"
)
add_dependencies(check-integration ${PROJECT_NAME})

add_custom_target(check-mission
    COMMAND ${CMAKE_CTEST_COMMAND} -L MissionManager --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running mission tests"
)
add_dependencies(check-mission ${PROJECT_NAME})

add_custom_target(check-vehicle
    COMMAND ${CMAKE_CTEST_COMMAND} -L Vehicle --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running vehicle tests"
)
add_dependencies(check-vehicle ${PROJECT_NAME})

add_custom_target(check-utilities
    COMMAND ${CMAKE_CTEST_COMMAND} -L Utilities --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running utility tests"
)
add_dependencies(check-utilities ${PROJECT_NAME})

add_custom_target(check-fast
    COMMAND ${CMAKE_CTEST_COMMAND} -LE Slow --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running fast tests"
)
add_dependencies(check-fast ${PROJECT_NAME})

add_custom_target(check-reliable
    COMMAND ${CMAKE_CTEST_COMMAND} -LE "Slow|Flaky" --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running reliable tests"
    VERBATIM
)
add_dependencies(check-reliable ${PROJECT_NAME})

add_custom_target(check-flaky
    COMMAND ${CMAKE_CTEST_COMMAND} -L Flaky --output-on-failure
    USES_TERMINAL
    COMMENT "Running flaky tests"
)
add_dependencies(check-flaky ${PROJECT_NAME})

add_custom_target(check-network
    COMMAND ${CMAKE_CTEST_COMMAND} -L Network --output-on-failure
    USES_TERMINAL
    COMMENT "Running network tests"
)
add_dependencies(check-network ${PROJECT_NAME})

add_custom_target(check-serial
    COMMAND ${CMAKE_CTEST_COMMAND}
        -R "ParameterManagerTest|MissionManagerTest|MissionWorkflowTest|RequestMessageTest|VehicleLinkManagerTest|FTPManagerTest"
        --output-on-failure
    USES_TERMINAL
    COMMENT "Running serialized tests"
)
add_dependencies(check-serial ${PROJECT_NAME})

add_custom_target(check-parallel
    COMMAND ${CMAKE_CTEST_COMMAND}
        -E "ParameterManagerTest|MissionManagerTest|MissionWorkflowTest|RequestMessageTest|VehicleLinkManagerTest|FTPManagerTest"
        --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running parallel tests"
)
add_dependencies(check-parallel ${PROJECT_NAME})

add_custom_target(check-ci
    COMMAND ${CMAKE_CTEST_COMMAND} -LE "Flaky|Network" --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running CI tests"
    VERBATIM
)
add_dependencies(check-ci ${PROJECT_NAME})

add_custom_target(check-ci-fast
    COMMAND ${CMAKE_CTEST_COMMAND} -LE "Slow|Flaky|Network" --output-on-failure --parallel ${QGC_TEST_PARALLEL_LEVEL}
    USES_TERMINAL
    COMMENT "Running fast CI tests"
    VERBATIM
)
add_dependencies(check-ci-fast ${PROJECT_NAME})

# ----------------------------------------------------------------------------
# add_qgc_test(name [LABELS ...] [TIMEOUT sec] [SERIAL] [RESOURCE_LOCK ...])
# ----------------------------------------------------------------------------

function(add_qgc_test test_name)
    cmake_parse_arguments(ARG "SERIAL" "TIMEOUT" "LABELS;RESOURCE_LOCK" ${ARGN})

    add_test(
        NAME ${test_name}
        COMMAND $<TARGET_FILE:${PROJECT_NAME}> --unittest:${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Timeout: explicit > Slow > Integration > Unit > default
    if(ARG_TIMEOUT)
        set(_timeout ${ARG_TIMEOUT})
    elseif("Slow" IN_LIST ARG_LABELS)
        set(_timeout ${QGC_TEST_TIMEOUT_SLOW})
    elseif("Integration" IN_LIST ARG_LABELS)
        set(_timeout ${QGC_TEST_TIMEOUT_INTEGRATION})
    elseif("Unit" IN_LIST ARG_LABELS)
        set(_timeout ${QGC_TEST_TIMEOUT_UNIT})
    else()
        set(_timeout ${QGC_TEST_TIMEOUT})
    endif()

    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT ${_timeout}
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false"
        FAIL_REGULAR_EXPRESSION "FAIL!;Segmentation fault;ASSERT"
    )

    if(ARG_LABELS)
        set_tests_properties(${test_name} PROPERTIES LABELS "${ARG_LABELS}")
    endif()

    # Resource locking: SERIAL > explicit RESOURCE_LOCK > Integration default
    if(ARG_SERIAL)
        set_tests_properties(${test_name} PROPERTIES
            RESOURCE_LOCK "MockLink;MissionController;FTPManager;ParameterManager;GlobalState"
            RUN_SERIAL TRUE
        )
    elseif(ARG_RESOURCE_LOCK)
        set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK "${ARG_RESOURCE_LOCK}")
    elseif("Integration" IN_LIST ARG_LABELS)
        set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK "MockLink")
    endif()
endfunction()
