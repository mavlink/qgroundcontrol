find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml QmlIntegration)

qt_add_library(AnalyzeView STATIC
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
    MAVLinkConsoleController.cc
    MAVLinkConsoleController.h
    PX4LogParser.cc
    PX4LogParser.h
    ULogParser.cc
    ULogParser.h
)

include(FetchContent)
FetchContent_Declare(ulogparser
    GIT_REPOSITORY https://github.com/PX4/ulog_cpp.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ulogparser)

target_link_libraries(AnalyzeView
    PRIVATE
        Qt6::Gui
        Qt6::Qml
        ulog_cpp::ulog_cpp
        FactSystem
        QGC
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
        MAVLink
        QmlControls
)

include(FetchContent)
FetchContent_Declare(easyexif
    GIT_REPOSITORY https://github.com/mayanklahiri/easyexif.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(easyexif)

target_sources(AnalyzeView
    PRIVATE
        ${easyexif_SOURCE_DIR}/exif.cpp
        ${easyexif_SOURCE_DIR}/exif.h
)

target_include_directories(AnalyzeView
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${easyexif_SOURCE_DIR}
)

# qt_add_qml_module(AnalyzeView
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
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts
#         QtQuick.Window
#         QtCharts
#         Qt.labs.qmlmodels
#         QGroundControl
#         QGroundControl.Palette
#         QGroundControl.Controls
#         QGroundControl.Controllers
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         QGroundControl.ScreenTools
#     DEPENDENCIES
#         QtCore
# )

find_package(Qt6 REQUIRED OPTIONAL_COMPONENTS Charts)
cmake_dependent_option(QGC_DISABLE_MAVLINK_INSPECTOR "Disable Mavlink Inspector" OFF "Qt6Charts_FOUND" ON) # This removes QtCharts which is GPL licensed
if(NOT QGC_DISABLE_MAVLINK_INSPECTOR)
    target_sources(AnalyzeView
        PRIVATE
            MAVLinkChartController.cc
            MAVLinkChartController.h
            MAVLinkInspectorController.cc
            MAVLinkInspectorController.h
            MAVLinkMessage.cc
            MAVLinkMessage.h
            MAVLinkMessageField.cc
            MAVLinkMessageField.h
            MAVLinkSystem.cc
            MAVLinkSystem.h
    )
    target_link_libraries(AnalyzeView
        PRIVATE
            Qt6::Charts
    )
else()
    target_compile_definitions(AnalyzeView PUBLIC QGC_DISABLE_MAVLINK_INSPECTOR)
endif()
