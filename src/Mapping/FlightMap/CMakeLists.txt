# ============================================================================
# Flight Map Module
# Map widgets, vehicle indicators, and mission visualization
# ============================================================================

qt_add_library(FlightMapModule STATIC)

qt_add_qml_module(
    FlightMapModule
    URI QGroundControl.FlightMap
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        MapItems/ADSBVehicleMapItem.qml
        MapItems/CameraTriggerIndicator.qml
        MapItems/CustomMapItems.qml
        MapItems/MapLineArrow.qml
        MapItems/MissionItemIndicator.qml
        MapItems/MissionItemIndicatorDrag.qml
        MapItems/MissionItemIndicatorGroup.qml
        MapItems/MissionItemSelectionPanel.qml
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
        MapAttribution.qml
        MapScale.qml
    NO_PLUGIN
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME} flightmap_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/gear-black.svg"
)

# subVehicleArrowOpaque.png alias for ArduSub firmware plugin (file lives here).
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/Images/sub.png"
    PROPERTIES
        QT_RESOURCE_ALIAS "subVehicleArrowOpaque.png"
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME} flightmap_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Images"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/adsbVehicle.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/AlertAircraft.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/attitudeDial.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/attitudeInstrument.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/attitudePointer.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/AwarenessAircraft.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/cOGPointer.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/compassDottedLine.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/compassInstrumentArrow.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/compassInstrumentDial.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/crossHair.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Help.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/HelpBlack.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/Home.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapAddMission.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapAddMissionBlack.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapCenter.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapCenterBlack.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapDrawShape.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapHome.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapHomeBlack.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapSync.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapSyncBlack.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapSyncChanged.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapType.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/MapTypeBlack.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/PiP.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/pipHide.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/pipResize.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/rollDialWhite.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/rollPointerWhite.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/scale.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/scale_end.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/scale_endLight.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/scaleLight.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/sub.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/vehicleArrowOpaque.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/vehicleArrowOutline.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/ZoomMinus.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/ZoomPlus.svg"
)
