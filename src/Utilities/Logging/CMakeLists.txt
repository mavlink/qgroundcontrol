# ============================================================================
# Logging Module
# Log management, disk writing, remote streaming, and category filtering
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LogCompression.cc
        LogCompression.h
        LogDiskWriter.cc
        LogDiskWriter.h
        LogFormatter.cc
        LogFormatter.h
        LogManager.cc
        LogManager.h
        LogModel.cc
        LogModel.h
        LogRemoteSink.cc
        LogRemoteSink.h
        QGCLogEntry.cc
        QGCLogEntry.h
        QGCLoggingCategory.cc
        QGCLoggingCategory.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
