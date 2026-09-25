# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GpsMavlinkTestHelpers.h GpsQmlTestHelpers.h GpsTestHelpers.h
                                             SequentialTestDevice.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(NMEA)

add_subdirectory(NTRIP)
add_subdirectory(Positioning)
add_subdirectory(RTCM)
add_subdirectory(Corrections)
add_subdirectory(Driver)
add_subdirectory(Receiver)
add_subdirectory(RTK)
add_subdirectory(Transport)

add_subdirectory(Core)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManagerTest.cc)
add_qgc_test(GPSManagerTest LABELS Unit GPS)

# Every GPS suite carries the GPS label so `ctest -L GPS` selects all of them. Reading another directory's test
# properties needs CMake 3.28.
function(qgc_check_gps_test_labels directory)
    get_property(
        tests
        DIRECTORY "${directory}"
        PROPERTY TESTS
    )
    foreach(test IN LISTS tests)
        get_property(
            labels
            TEST "${test}"
            DIRECTORY "${directory}"
            PROPERTY LABELS
        )
        if(NOT "GPS" IN_LIST labels)
            message(FATAL_ERROR "GPS test ${test} in ${directory} needs the GPS label")
        endif()
    endforeach()
    get_property(
        subdirectories
        DIRECTORY "${directory}"
        PROPERTY SUBDIRECTORIES
    )
    foreach(subdirectory IN LISTS subdirectories)
        qgc_check_gps_test_labels("${subdirectory}")
    endforeach()
endfunction()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
    qgc_check_gps_test_labels("${CMAKE_CURRENT_SOURCE_DIR}")
endif()
