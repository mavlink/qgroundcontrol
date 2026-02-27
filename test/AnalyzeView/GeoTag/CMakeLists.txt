# ============================================================================
# GeoTag Unit Tests
# Tests for geotagging, EXIF parsing, and log parsing
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DataFlashParserTest.cc
        DataFlashParserTest.h
        DataFlashTestGenerator.cc
        DataFlashTestGenerator.h
        ExifParserTest.cc
        ExifParserTest.h
        GeoTagControllerTest.cc
        GeoTagControllerTest.h
        GeoTagDataTest.cc
        GeoTagDataTest.h
        GeoTagImageModelTest.cc
        GeoTagImageModelTest.h
        ULogParserTest.cc
        ULogParserTest.h
        ULogTestGenerator.cc
        ULogTestGenerator.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
# EXIF test sample from https://github.com/ianare/exif-samples
qt_add_resources(${CMAKE_PROJECT_NAME} "GeoTagTest_res"
    PREFIX "/unittest"
    FILES
        DSCN0010.jpg
)
