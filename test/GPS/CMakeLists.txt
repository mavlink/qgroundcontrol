# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GpsTestHelpers.h SequentialTestDevice.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(NMEA)

add_subdirectory(NTRIP)
add_subdirectory(PositionManager)
add_subdirectory(RTCM)
add_subdirectory(RTK)
add_subdirectory(Transport)

add_subdirectory(Core)
add_subdirectory(Standalone)
