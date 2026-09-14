# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GpsTestHelpers.h NmeaSourceManagerTest.cc NmeaSourceManagerTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(NmeaSourceManagerTest LABELS Unit RESOURCE_LOCK Settings)

add_subdirectory(NTRIP)
add_subdirectory(PositionManager)
add_subdirectory(RTCM)
add_subdirectory(RTK)

add_subdirectory(Core)
add_subdirectory(Standalone)
