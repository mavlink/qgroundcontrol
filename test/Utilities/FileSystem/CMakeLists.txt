# ============================================================================
# FileSystem Utilities Unit Tests
# Tests for file download and filesystem operations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCArchiveWatcherTest.cc
        QGCArchiveWatcherTest.h
        QGCFileDownloadTest.cc
        QGCFileDownloadTest.h
        QGCFileHelperTest.cc
        QGCFileHelperTest.h
        QGCFileWatcherTest.cc
        QGCFileWatcherTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
