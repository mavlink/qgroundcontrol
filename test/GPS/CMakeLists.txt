# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GpsTestHelpers.h Replay/GPSReplayScheduler.cc Replay/GPSReplayScheduler.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManagerTest.cc GPSManagerTest.h GPSSettingsTest.cc GPSSettingsTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Replay)

add_qgc_test(GPSManagerTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(GPSSettingsTest LABELS Unit RESOURCE_LOCK Settings)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDiagnosticsUITest.cc GPSDiagnosticsUITest.h)
add_qgc_test(GPSDiagnosticsUITest LABELS Unit RESOURCE_LOCK Settings)

add_subdirectory(Contracts)
add_subdirectory(Core)
add_subdirectory(NMEA)
add_subdirectory(NTRIP)
add_subdirectory(PositionManager)
add_subdirectory(Corrections)
add_subdirectory(Models)
add_subdirectory(Driver)
add_subdirectory(Receiver)
add_subdirectory(BaseStation)
add_subdirectory(Recording)
add_subdirectory(Replay)
add_subdirectory(Fuzz)
