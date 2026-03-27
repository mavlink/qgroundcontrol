# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSEventModelTest.cc
        GPSEventModelTest.h
        GPSFormatterTest.cc
        GPSFormatterTest.h
        GPSManagerTest.cc
        GPSManagerTest.h
        GPSPositionSourceTest.cc
        GPSPositionSourceTest.h
        GPSProviderTest.cc
        GPSProviderTest.h
        GPSSatelliteInfoSourceTest.cc
        GPSSatelliteInfoSourceTest.h
        GpsTestHelpers.h
        GPSTransportTest.cc
        GPSTransportTest.h
        RTCMTestHelper.h
        VehicleGPSAggregateFactGroupTest.cc
        VehicleGPSAggregateFactGroupTest.h
        VehicleGPSFactGroupTest.cc
        VehicleGPSFactGroupTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RTK)
add_subdirectory(NTRIP)
