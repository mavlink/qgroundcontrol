add_qgc_tests(
    QGCArchiveModelTest
    QGCCompressionTest
    QGCStreamingDecompressionTest
    LABELS Unit Utilities
)

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
