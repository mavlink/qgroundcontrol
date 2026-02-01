# ============================================================================
# Utilities Module
# Core utilities, logging, file system helpers, and platform abstractions
# ============================================================================

# ----------------------------------------------------------------------------
# Core Utility Sources
# ----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DeviceInfo.cc
        DeviceInfo.h
        JsonHelper.cc
        JsonHelper.h
        FileSystem/QGCCachedFileDownload.cc
        FileSystem/QGCCachedFileDownload.h
        FileSystem/QGCFileDownload.cc
        FileSystem/QGCFileDownload.h
        FileSystem/QGCFileHelper.cc
        FileSystem/QGCFileHelper.h
        FileSystem/QGCArchiveWatcher.cc
        FileSystem/QGCArchiveWatcher.h
        FileSystem/QGCFileWatcher.cc
        FileSystem/QGCFileWatcher.h
        QGC.cc
        QGC.h
        QGCCommandLineParser.cc
        QGCCommandLineParser.h
        QGCLogging.cc
        QGCLogging.h
        QGCLoggingCategory.cc
        QGCLoggingCategory.h
        StateMachine.cc
        StateMachine.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        Compression
        FileSystem
        Parsing
        Parsing/DataFlash
        Parsing/Exif
        Parsing/ULog
)

# ============================================================================
# Utility Subdirectories
# ============================================================================

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
add_subdirectory(Network)
add_subdirectory(Parsing)
add_subdirectory(Platform)
add_subdirectory(SDL)
add_subdirectory(StateMachine)
