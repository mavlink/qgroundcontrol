find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(UI STATIC)

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/toolbar/Images/*.*)
qt_add_qml_module(UI
    URI QGroundControl.UI
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    RESOURCES ${QML_RESOURCES}
    OUTPUT_TARGETS UI_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
        QtQuick
        QtQuick.Controls
        QtQuick.Dialogs
        QtQuick.Layouts
        QtQuick.Window

        QGroundControl
        QGroundControl.Palette
        QGroundControl.Controls
        QGroundControl.ScreenTools
        QGroundControl.FlightDisplay
        QGroundControl.FlightMap
)
