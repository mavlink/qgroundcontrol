find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(PlanView STATIC)

file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
qt_add_qml_module(PlanView
    URI QGroundControl.PlanView
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    OUTPUT_TARGETS PlanView_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
        QtQuick
        QtQuick.Controls
        QtLocation
        QtPositioning

        QGroundControl
        QGroundControl.ScreenTools
        QGroundControl.Palette
        QGroundControl.Controls
        QGroundControl.FlightMap
)
