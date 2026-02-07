# ============================================================================
# Logging Module
# Log management, disk writing, remote streaming, and category filtering
# ============================================================================

# ============================================================================
# Compile-time Qt Log Filtering
# ============================================================================

if(QGC_DISABLE_QT_DEBUG)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QT_NO_DEBUG_OUTPUT)
    message(STATUS "Qt debug output disabled at compile time")
endif()

if(QGC_DISABLE_QT_INFO)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QT_NO_INFO_OUTPUT)
    message(STATUS "Qt info output disabled at compile time")
endif()

# ============================================================================
# Source Files
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LogCompression.cc
        LogCompression.h
        LogFormatter.cc
        LogFormatter.h
        LogManager.cc
        LogManager.h
        LogModel.cc
        LogModel.h
        LogRemoteSink.cc
        LogRemoteSink.h
        LogEntry.cc
        LogEntry.h
        LoggingCategoryItem.cc
        LoggingCategoryItem.h
        LoggingCategoryManager.cc
        LoggingCategoryManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
