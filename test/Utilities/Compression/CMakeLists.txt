# ============================================================================
# Compression Utilities Unit Tests
# Tests for QGCCompression (format detection, ZIP, GZIP, XZ)
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        QGCArchiveModelTest.cc
        QGCArchiveModelTest.h
        QGCCompressionTest.cc
        QGCCompressionTest.h
        QGCStreamingDecompressionTest.cc
        QGCStreamingDecompressionTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------

# Core test files (always included)
set(_compression_test_files manifest.json.gz manifest.json.xz manifest.json.zip manifest.json.7z manifest.json.zst)

# Optional format test files
if(QGC_ENABLE_BZIP2)
    list(APPEND _compression_test_files manifest.json.bz2)
endif()
if(QGC_ENABLE_LZ4)
    list(APPEND _compression_test_files manifest.json.lz4)
endif()

qt_add_resources(
    ${CMAKE_PROJECT_NAME} "UtilitiesTest_CompressionTest_res"
    PREFIX "/unittest"
    FILES
        ${_compression_test_files}
)

add_qgc_test(QGCArchiveModelTest LABELS Unit Utilities)
add_qgc_test(QGCCompressionTest LABELS Unit Utilities RESOURCE_LOCK TempFiles)
add_qgc_test(QGCStreamingDecompressionTest LABELS Unit Utilities)
