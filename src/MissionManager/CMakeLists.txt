set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/BlankPlanCreator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/BlankPlanCreator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraCalc.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraCalc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraSection.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraSection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraSpec.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraSpec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ComplexMissionItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ComplexMissionItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CorridorScanComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CorridorScanComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CorridorScanPlanCreator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CorridorScanPlanCreator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FixedWingLandingComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FixedWingLandingComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoFenceController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoFenceController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoFenceManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoFenceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/KMLPlanDomDocument.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/KMLPlanDomDocument.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LandingComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LandingComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandList.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandList.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandTree.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandUIInfo.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandUIInfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionSettingsItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionSettingsItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanCreator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanCreator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanElementController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanElementController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanMasterController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanMasterController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RallyPoint.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RallyPointController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RallyPointController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RallyPoint.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RallyPointManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RallyPointManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Section.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleMissionItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleMissionItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SpeedSection.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SpeedSection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StructureScanComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/StructureScanComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StructureScanPlanCreator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/StructureScanPlanCreator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SurveyComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SurveyComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SurveyPlanCreator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SurveyPlanCreator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TakeoffMissionItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TakeoffMissionItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransectStyleComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TransectStyleComplexItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VisualMissionItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VisualMissionItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VTOLLandingComplexItem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VTOLLandingComplexItem.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

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
