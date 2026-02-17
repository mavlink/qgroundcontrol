# ============================================================================
# EXIF Utilities Unit Tests
# Tests for ExifUtility (metadata reading/writing using libexif)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ExifUtilityTest.cc
        ExifUtilityTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Note: Test image DSCN0010.jpg is provided by test/AnalyzeView resources
