# ============================================================================
# Compression Utilities Unit Tests
# Tests for QGCCompression (format detection, ZIP, GZIP, XZ)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCCompressionTest.cc
        QGCCompressionTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_CompressionTest_res"
    PREFIX "/unittest"
    FILES
        manifest.json.gz
        manifest.json.xz
        manifest.json.zip
)
