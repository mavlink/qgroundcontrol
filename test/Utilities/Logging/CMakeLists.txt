# ============================================================================
# Logging Utilities Unit Tests
# Tests for log management and remote streaming
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LogCompressionTest.cc
        LogCompressionTest.h
        LogEntryTest.cc
        LogEntryTest.h
        LogFormatterTest.cc
        LogFormatterTest.h
        LogManagerTest.cc
        LogManagerTest.h
        LogModelTest.cc
        LogModelTest.h
        LogRemoteSinkTest.cc
        LogRemoteSinkTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
