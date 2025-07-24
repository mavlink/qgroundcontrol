qt_add_library(FlightMapModule STATIC)

qt_add_qml_module(FlightMapModule
    URI QGroundControl.FlightMap
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        MapItems/CameraTriggerIndicator.qml
        MapItems/CustomMapItems.qml
        MapItems/MapLineArrow.qml
        MapItems/MissionItemIndicator.qml
        MapItems/MissionItemIndicatorDrag.qml
        MapItems/MissionLineView.qml
        MapItems/PlanMapItems.qml
        MapItems/ProximityRadarMapView.qml
        MapItems/QGCMapCircleVisuals.qml
        MapItems/QGCMapPolygonVisuals.qml
        MapItems/QGCMapPolylineVisuals.qml
        MapItems/SplitIndicator.qml
        MapItems/VehicleMapItem.qml
        Widgets/CenterMapDropButton.qml
        Widgets/CenterMapDropPanel.qml
        Widgets/CompassDial.qml
        Widgets/CompassHeadingIndicator.qml
        Widgets/HorizontalCompassAttitude.qml
        Widgets/IntegratedAttitudeIndicator.qml
        Widgets/IntegratedCompassAttitude.qml
        Widgets/MapFitFunctions.qml
        Widgets/PhotoVideoControl.qml
        Widgets/QGCArtificialHorizon.qml
        Widgets/QGCAttitudeWidget.qml
        Widgets/QGCCompassWidget.qml
        Widgets/QGCPitchIndicator.qml
        Widgets/VerticalCompassAttitude.qml
        FlightMap.qml
        MapScale.qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)
