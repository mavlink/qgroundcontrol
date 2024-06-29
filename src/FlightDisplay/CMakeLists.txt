find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FlightDisplay STATIC)

file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
qt_add_qml_module(FlightDisplay
    URI QGroundControl.FlightDisplay
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    OUTPUT_TARGETS FlightDisplay_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
        QtQuick
        QtQuick.Controls
        QtQuick.Dialogs
        QtQuick.Layouts

        QtLocation
        QtPositioning
        QtQuick.Window
        QtQml.Models

        QGroundControl
        QGroundControl.Controllers
        QGroundControl.Controls
        QGroundControl.FactSystem
        QGroundControl.FlightDisplay
        QGroundControl.FlightMap
        QGroundControl.Palette
        QGroundControl.ScreenTools
        QGroundControl.Vehicle
)
