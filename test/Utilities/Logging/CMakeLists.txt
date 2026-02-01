# ============================================================================
# Logging Utilities Unit Tests
# Tests for log management, disk writing, and remote streaming
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LogCompressionTest.cc
        LogCompressionTest.h
        LogDiskWriterTest.cc
        LogDiskWriterTest.h
        LogManagerTest.cc
        LogManagerTest.h
        LogModelTest.cc
        LogModelTest.h
        LogRemoteSinkTest.cc
        LogRemoteSinkTest.h
        QGCLogEntryTest.cc
        QGCLogEntryTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
