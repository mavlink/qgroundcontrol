# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GpsTestHelpers.h UdpForwarderTest.cc UdpForwarderTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManagerTest.cc GPSManagerTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionStateTest.cc GPSConnectionStateTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSourceHealthTest.cc GPSSourceHealthTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionManagerTest.cc GPSCorrectionManagerTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSManagerTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(GPSConnectionStateTest LABELS Unit)
add_qgc_test(GPSSourceHealthTest LABELS Unit)
add_qgc_test(GPSCorrectionManagerTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(UdpForwarderTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionConfigTest.cc GPSConnectionConfigTest.h)
add_qgc_test(GPSConnectionConfigTest LABELS Unit RESOURCE_LOCK Settings)

add_subdirectory(NMEA)
add_subdirectory(NTRIP)
add_subdirectory(PositionManager)
add_subdirectory(RTCM)
add_subdirectory(RTK)
