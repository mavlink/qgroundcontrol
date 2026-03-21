# ============================================================================
# Analyze View Unit Tests
# Tests for log analysis and telemetry visualization
# ============================================================================

add_subdirectory(GeoTag)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MAVLinkChartControllerTest.cc
        MAVLinkChartControllerTest.h
        MAVLinkConsoleControllerTest.cc
        MAVLinkConsoleControllerTest.h
        MAVLinkInspectorControllerTest.cc
        MAVLinkInspectorControllerTest.h
        OnboardLogDownloadTest.cc
        OnboardLogDownloadTest.h
        MAVLinkMessageFieldTest.cc
        MAVLinkMessageFieldTest.h
        MAVLinkMessageTest.cc
        MAVLinkMessageTest.h
        MAVLinkSystemTest.cc
        MAVLinkSystemTest.h
        MavlinkLogTest.cc
        MavlinkLogTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

