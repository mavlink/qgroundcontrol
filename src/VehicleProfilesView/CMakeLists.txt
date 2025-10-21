# ============================================================================
# Vehicle Profiles View Module
# UI components for managing vehicle profile settings
# ============================================================================

qt_add_library(VehicleProfilesViewModule STATIC)

qt_add_qml_module(VehicleProfilesViewModule
    URI QGroundControl.VehicleProfilesView
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        VehicleProfilesList.qml
        VehicleProfilesView.qml
    NO_PLUGIN
)
