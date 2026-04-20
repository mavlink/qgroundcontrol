# ============================================================================
# Analyze View Module
# Flight log analysis, MAVLink inspection, and telemetry visualization
# ============================================================================

add_subdirectory(GeoTag)
add_subdirectory(MAVLinkConsole)
add_subdirectory(MAVLinkInspector)
add_subdirectory(OnboardLogs)
add_subdirectory(OnboardLogsFtp)

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
        OnboardLogsFtp/OnboardLogFtpPage.qml
        Vibration/VibrationPage.qml
    NO_PLUGIN
)

# Flatten subdir paths into /qmlimages/<name>; VibrationPageIcon URL has no .png
# extension (consumer is QGCCorePlugin.cc).
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/GeoTag/GeoTagIcon.svg"
    PROPERTIES QT_RESOURCE_ALIAS "GeoTagIcon.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkConsole/MAVLinkConsoleIcon.svg"
    PROPERTIES QT_RESOURCE_ALIAS "MAVLinkConsoleIcon.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkInspector/MAVLinkInspector.svg"
    PROPERTIES QT_RESOURCE_ALIAS "MAVLinkInspector.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/OnboardLogs/OnboardLogIcon.svg"
    PROPERTIES QT_RESOURCE_ALIAS "OnboardLogIcon.svg")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Vibration/VibrationPageIcon.png"
    PROPERTIES QT_RESOURCE_ALIAS "VibrationPageIcon")

qt_add_resources(${CMAKE_PROJECT_NAME} analyzeview_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/FloatingWindow.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/GeoTag/GeoTagIcon.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkConsole/MAVLinkConsoleIcon.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkInspector/MAVLinkInspector.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/OnboardLogs/OnboardLogIcon.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vibration/VibrationPageIcon.png"
)
