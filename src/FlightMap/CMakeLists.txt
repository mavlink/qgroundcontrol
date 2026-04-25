# ============================================================================
# Flight Map Module
# Map widgets, vehicle indicators, and mission visualization
# ============================================================================

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
    NO_PLUGIN
)

qt_add_resources(${CMAKE_PROJECT_NAME} flightmap_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/gear-black.svg"
)

# subVehicleArrowOpaque.png alias for ArduSub firmware plugin (file lives here).
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/FlightMap/Images/sub.png"
    PROPERTIES QT_RESOURCE_ALIAS "subVehicleArrowOpaque.png")

qt_add_resources(${CMAKE_PROJECT_NAME} flightmap_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_SOURCE_DIR}/src/FlightMap/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/adsbVehicle.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/AlertAircraft.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/attitudeDial.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/attitudeInstrument.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/attitudePointer.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/AwarenessAircraft.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/cOGPointer.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/compassDottedLine.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/compassInstrumentArrow.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/compassInstrumentDial.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/crossHair.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/Help.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/HelpBlack.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/Home.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapAddMission.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapAddMissionBlack.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapCenter.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapCenterBlack.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapDrawShape.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapHome.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapHomeBlack.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapSync.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapSyncBlack.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapSyncChanged.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapType.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/MapTypeBlack.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/PiP.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/pipHide.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/pipResize.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/rollDialWhite.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/rollPointerWhite.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/scale.png"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/scale_end.png"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/scale_endLight.png"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/scaleLight.png"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/sub.png"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/vehicleArrowOpaque.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/vehicleArrowOutline.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/ZoomMinus.svg"
        "${CMAKE_SOURCE_DIR}/src/FlightMap/Images/ZoomPlus.svg"
)
