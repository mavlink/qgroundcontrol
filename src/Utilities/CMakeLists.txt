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
        FileSystem/QGCTemporaryFile.cc
        FileSystem/QGCTemporaryFile.h
        Platform.cc
        Platform.h
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

# ----------------------------------------------------------------------------
# Platform-Specific Sources
# ----------------------------------------------------------------------------
# Signal handling not available on mobile platforms
if(NOT ANDROID AND NOT IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            SignalHandler.cc
            SignalHandler.h
    )
endif()

# iOS-specific screen management
if(IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            MobileScreenMgr.mm
            MobileScreenMgr.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        FileSystem
)

# ============================================================================
# Utility Subdirectories
# ============================================================================

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
add_subdirectory(StateMachine)
