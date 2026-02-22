# ============================================================================
# Analyze View Module
# Flight log analysis, MAVLink inspection, and telemetry visualization
# ============================================================================

add_subdirectory(GeoTag)
add_subdirectory(LogAnalysis)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LogDownloadController.cc
        LogDownloadController.h
        LogEntry.cc
        LogEntry.h
        MAVLinkChartController.cc
        MAVLinkChartController.h
        MAVLinkConsoleController.cc
        MAVLinkConsoleController.h
        MAVLinkInspectorController.cc
        MAVLinkInspectorController.h
        MAVLinkMessage.cc
        MAVLinkMessage.h
        MAVLinkMessageField.cc
        MAVLinkMessageField.h
        MAVLinkSystem.cc
        MAVLinkSystem.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# QML Module for Analyze View UI
# ----------------------------------------------------------------------------
qt_add_library(AnalyzeViewModule STATIC)

qt_add_qml_module(AnalyzeViewModule
    URI QGroundControl.AnalyzeView
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        AnalyzeView.qml
        LogAnalysis/LogAnalysisPage.qml
        LogDownloadPage.qml
        MAVLinkConsolePage.qml
        MAVLinkInspectorPage.qml
        VibrationPage.qml
    RESOURCES
        LogAnalysis/LogAnalysisIcon.svg
    NO_PLUGIN
)
