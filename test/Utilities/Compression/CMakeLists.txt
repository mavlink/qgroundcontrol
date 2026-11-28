# ============================================================================
# Compression Utilities Unit Tests
# QGCCompression: file kind detection, single streams, archive extraction, and the job
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        ArchiveFixtures.cc
        ArchiveFixtures.h
        Fuzz/BidAgreement.cc
        Fuzz/BidAgreement.h
        QGCArchiveExtractionTest.cc
        QGCArchiveExtractionTest.h
        QGCCompressionJobTest.cc
        QGCCompressionJobTest.h
        QGCCompressionTest.cc
        QGCCompressionTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# BidAgreement compares the private bid rules of QGCCompression with libarchive itself
include(${CMAKE_CURRENT_SOURCE_DIR}/Fuzz/BidAgreement.cmake)
qgc_add_bid_agreement(QGCUnitTests)

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/CompressionTestResources.cmake)
qgc_add_compression_test_resources(${CMAKE_PROJECT_NAME})

add_qgc_test(QGCArchiveExtractionTest LABELS Unit Utilities)
add_qgc_test(QGCCompressionJobTest LABELS Unit Utilities)
add_qgc_test(QGCCompressionTest LABELS Unit Utilities)

add_subdirectory(Fuzz)
