# ============================================================================
# Unified Log Viewer Module
# Shared shell for .tlog, .bin (DataFlash), and .ulg (PX4 ULog) log workflows
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        APMDataFlash/APMDataFlashLogParser.cc
        APMDataFlash/APMDataFlashLogParser.h
        APMDataFlash/LogViewerDataFlashParser.cc
        APMDataFlash/LogViewerDataFlashParser.h
        LogFileParser.cc
        LogFileParser.h
        LogParseResultPrivate.h
        LogViewerController.cc
        LogViewerController.h
        PX4ULog/ULogFullHandler.cc
        PX4ULog/ULogFullHandler.h
        PX4ULog/LogViewerULogParser.cc
        PX4ULog/LogViewerULogParser.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/APMDataFlash
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4ULog
)
