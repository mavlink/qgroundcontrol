# ============================================================================
# Analyze View Unit Tests
# Tests for log analysis, EXIF parsing, and telemetry visualization
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ExifParserTest.cc
        ExifParserTest.h
        # GeoTagControllerTest.cc
        # GeoTagControllerTest.h
        LogDownloadTest.cc
        LogDownloadTest.h
        MavlinkLogTest.cc
        MavlinkLogTest.h
        PX4LogParserTest.cc
        PX4LogParserTest.h
        # ULogParserTest.cc
        # ULogParserTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
# EXIF test samples from https://github.com/ianare/exif-samples
qt_add_resources(${CMAKE_PROJECT_NAME} "AnalyzeViewTest_res"
    PREFIX "/unittest"
    FILES
        DSCN0010.jpg
        # SampleULog.ulg
)
