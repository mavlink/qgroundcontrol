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
        APMDataFlashLogParserTest.cc
        APMDataFlashLogParserTest.h
        LogFileParserTest.cc
        LogFileParserTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(MAVLinkChartControllerTest LABELS Unit AnalyzeView)
add_qgc_test(MAVLinkConsoleControllerTest LABELS Unit AnalyzeView)
add_qgc_test(MAVLinkInspectorControllerTest LABELS Unit AnalyzeView)
add_qgc_test(OnboardLogDownloadTest LABELS Integration AnalyzeView Vehicle)
add_qgc_test(MAVLinkMessageFieldTest LABELS Unit AnalyzeView)
add_qgc_test(MAVLinkMessageTest LABELS Unit AnalyzeView)
add_qgc_test(MAVLinkSystemTest LABELS Unit AnalyzeView)
add_qgc_test(MavlinkLogTest LABELS Integration AnalyzeView Vehicle)
add_qgc_test(APMDataFlashLogParserTest LABELS Unit AnalyzeView)
add_qgc_test(LogFileParserTest LABELS Unit AnalyzeView)
