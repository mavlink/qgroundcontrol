qt_add_library(DroneListModule STATIC)

qt_add_qml_module(DroneListModule
    URI QGroundControl.DroneList
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        DroneList.qml
        DroneDetail.qml
    NO_PLUGIN
)
