find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FlightMap STATIC)

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Images/*)
qt_add_qml_module(FlightMap
    URI QGroundControl.FlightMap
    VERSION 1.0
    RESOURCES ${QML_RESOURCES}
    QML_FILES ${QML_SOURCES}
    OUTPUT_TARGETS FlightMap_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
        QtQuick
        QtQuick.Controls
        QtLocation
        QtPositioning
        QtQuick.Dialogs

        QGroundControl
        QGroundControl.FactSystem
        QGroundControl.Controls
        QGroundControl.FlightMap
        QGroundControl.ScreenTools
        QGroundControl.MultiVehicleManager
        QGroundControl.Vehicle
        QGroundControl.QGCPositionManager
)
