# ============================================================================
# Flight Map Module - Minimal Map Only
# ============================================================================

qt_add_library(FlightMapModule STATIC)

qt_add_qml_module(FlightMapModule
    URI QGroundControl.FlightMap
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        FlightMap.qml
        MapScale.qml
    NO_PLUGIN
)
