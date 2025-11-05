# ============================================================================
# Analyze View Module
# Flight log analysis, MAVLink inspection, and telemetry visualization
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ExifParser.cc
        ExifParser.h
        GeoTagController.cc
        GeoTagController.h
        GeoTagWorker.cc
        GeoTagWorker.h
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
        PX4LogParser.cc
        PX4LogParser.h
        ULogParser.cc
        ULogParser.h
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
        GeoTagPage.qml
        LogDownloadPage.qml
        MAVLinkConsolePage.qml
        MAVLinkInspectorPage.qml
        VibrationPage.qml
    NO_PLUGIN
)

# ============================================================================
# ULog Parser Integration
# ============================================================================

CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG main
)

# Suppress Clang warnings for ulog_cpp
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ulog_cpp PRIVATE -Wno-unknown-warning-option)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ulog_cpp::ulog_cpp)
