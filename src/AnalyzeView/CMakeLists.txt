# ============================================================================
# Analyze View Module
# Flight log analysis, MAVLink inspection, and telemetry visualization
# ============================================================================

add_subdirectory(GeoTag)
add_subdirectory(MAVLinkConsole)
add_subdirectory(MAVLinkInspector)
add_subdirectory(OnboardLogs)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
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
        AnalyzePage.qml
        AnalyzeView.qml
        MAVLinkConsole/MAVLinkConsolePage.qml
        MAVLinkInspector/MAVLinkChart.qml
        MAVLinkInspector/MAVLinkMessageButton.qml
        MAVLinkInspector/MAVLinkInspectorPage.qml
        OnboardLogs/OnboardLogPage.qml
        Vibration/VibrationPage.qml
    NO_PLUGIN
)
