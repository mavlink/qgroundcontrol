find_package(Qt6 REQUIRED COMPONENTS Core Gui Positioning Qml Xml)

target_sources(QGC
    PRIVATE
        BlankPlanCreator.cc
        BlankPlanCreator.h
        CameraCalc.cc
        CameraCalc.h
        CameraSection.cc
        CameraSection.h
        CameraSpec.cc
        CameraSpec.h
        ComplexMissionItem.cc
        ComplexMissionItem.h
        CorridorScanComplexItem.cc
        CorridorScanComplexItem.h
        CorridorScanPlanCreator.cc
        CorridorScanPlanCreator.h
        FixedWingLandingComplexItem.cc
        FixedWingLandingComplexItem.h
        GeoFenceController.cc
        GeoFenceController.h
        GeoFenceManager.cc
        GeoFenceManager.h
        KMLPlanDomDocument.cc
        KMLPlanDomDocument.h
        LandingComplexItem.cc
        LandingComplexItem.h
        MissionCommandList.cc
        MissionCommandList.h
        MissionCommandTree.cc
        MissionCommandTree.h
        MissionCommandUIInfo.cc
        MissionCommandUIInfo.h
        MissionController.cc
        MissionController.h
        MissionItem.cc
        MissionItem.h
        MissionManager.cc
        MissionManager.h
        MissionSettingsItem.cc
        MissionSettingsItem.h
        PlanCreator.cc
        PlanCreator.h
        PlanElementController.cc
        PlanElementController.h
        PlanManager.cc
        PlanManager.h
        PlanMasterController.cc
        PlanMasterController.h
        RallyPoint.cc
        RallyPointController.cc
        RallyPointController.h
        RallyPoint.h
        RallyPointManager.cc
        RallyPointManager.h
        Section.h
        SimpleMissionItem.cc
        SimpleMissionItem.h
        SpeedSection.cc
        SpeedSection.h
        StructureScanComplexItem.cc
        StructureScanComplexItem.h
        StructureScanPlanCreator.cc
        StructureScanPlanCreator.h
        SurveyComplexItem.cc
        SurveyComplexItem.h
        SurveyPlanCreator.cc
        SurveyPlanCreator.h
        TakeoffMissionItem.cc
        TakeoffMissionItem.h
        TransectStyleComplexItem.cc
        TransectStyleComplexItem.h
        VisualMissionItem.cc
        VisualMissionItem.h
        VTOLLandingComplexItem.cc
        VTOLLandingComplexItem.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
        Qt6::Xml
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(QGC
#     URI QGroundControl.MissionManager
#     VERSION 1.0
#     RESOURCES
#       BlankPlanCreator.png
#       CogWheel.svg
#       StructureScanPlanCreator.png
#       SurveyPlanCreator.png
#     QML_FILES
#       MapLineArrow.qml
#       QGCMapCircleVisuals.qml
#       QGCMapPolygonVisuals.qml
#       QGCMapPolylineVisuals.qml
#     OUTPUT_TARGETS MissionManager_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#       QtQuick
#       QtQuick.Controls
#       QtLocation
#       QtPositioning

#       QGroundControl
#       QGroundControl.ScreenTools
#       QGroundControl.Palette
#       QGroundControl.Controls
#       QGroundControl.FlightMap
# )
