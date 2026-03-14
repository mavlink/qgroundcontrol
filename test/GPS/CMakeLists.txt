# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSEventModelTest.cc
        GPSEventModelTest.h
        GPSManagerTest.cc
        GPSManagerTest.h
        GPSPositionSourceTest.cc
        GPSPositionSourceTest.h
        GPSProviderTest.cc
        GPSProviderTest.h
        GPSTransportTest.cc
        GPSTransportTest.h
        RTCMTestHelper.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RTK)
add_subdirectory(NTRIP)
