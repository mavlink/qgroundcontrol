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

# qt_add_qml_module(QGC
#     URI QGroundControl.AnalyzeView
#     VERSION 1.0
#     QML_FILES
#         AnalyzePage.qml
#         AnalyzeView.qml
#         GeoTagPage.qml
#         LogDownloadPage.qml
#         MAVLinkConsolePage.qml
#         MAVLinkInspectorPage.qml
#         VibrationPage.qml
#     RESOURCES
#         FloatingWindow.svg
#         GeoTagIcon.svg
#         LogDownloadIcon.svg
#         MAVLinkConsoleIcon.svg
#         MAVLinkInspector.svg
#         VibrationPageIcon.png
#     OUTPUT_TARGETS AnalyzeView_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QGroundControl
#         QGroundControl.Controllers
#         QGroundControl.Controls
#         QGroundControl.FactControls
#         QGroundControl.FactSystem
#         QGroundControl.Palette
#         QGroundControl.ScreenTools
#         Qt.labs.qmlmodels
#         QtCharts
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts
#         QtQuick.Window
#     DEPENDENCIES
#         QtCore
# )

#===========================================================================#

CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG main
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ulog_cpp PRIVATE -Wno-unknown-warning-option)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ulog_cpp::ulog_cpp)
