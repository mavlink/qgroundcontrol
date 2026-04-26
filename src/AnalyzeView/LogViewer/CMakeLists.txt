# ============================================================================
# Unified Log Viewer Module
# Shared shell for .tlog and .bin log workflows
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DataFlash/DataFlashLogParser.cc
        DataFlash/DataFlashLogParser.h
        LogViewerController.cc
        LogViewerController.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/DataFlash
)
