find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FirstRunPromptDialogs STATIC)

qt_add_qml_module(FirstRunPromptDialogs
    URI QGroundControl.FirstRunPromptDialogs
    VERSION 1.0
    QML_FILES
        FirstRunPrompt.qml
        OfflineVehicleFirstRunPrompt.qml
        UnitsFirstRunPrompt.qml
    OUTPUT_TARGETS FirstRunPromptDialogs_targets
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
