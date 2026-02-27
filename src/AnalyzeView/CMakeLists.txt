# ============================================================================
# Analyze View Module
# Flight log analysis, MAVLink inspection, and telemetry visualization
# ============================================================================

add_subdirectory(GeoTag)

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
        LogDownloadPage.qml
        MAVLinkConsolePage.qml
        MAVLinkInspectorPage.qml
        VibrationPage.qml
    NO_PLUGIN
)

#===========================================================================#

if(NOT USE_SYSTEM_ULOG_CPP)
CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG main
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ulog_cpp PRIVATE -Wno-unknown-warning-option)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ulog_cpp::ulog_cpp)
else()
find_package(ulog_cpp REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ulog_cpp)
endif()
