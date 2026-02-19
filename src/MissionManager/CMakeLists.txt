# ============================================================================
# Mission Manager Module
# Handles mission planning, geofences, rally points, and survey patterns
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
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

        # PlanShapes - Core
        PlanShapes/GeoFormatPlanImporter.cc
        PlanShapes/GeoFormatPlanImporter.h
        PlanShapes/PlanDocument.cc
        PlanShapes/PlanDocument.h
        PlanShapes/PlanExporter.cc
        PlanShapes/PlanExporter.h
        PlanShapes/PlanImporter.cc
        PlanShapes/PlanImporter.h

        # PlanShapes - CSV format
        PlanShapes/CSV/CsvPlanDocument.cc
        PlanShapes/CSV/CsvPlanDocument.h
        PlanShapes/CSV/CsvPlanExporter.cc
        PlanShapes/CSV/CsvPlanExporter.h
        PlanShapes/CSV/CsvPlanImporter.cc
        PlanShapes/CSV/CsvPlanImporter.h

        # PlanShapes - GeoJSON format
        PlanShapes/GeoJSON/GeoJsonPlanDocument.cc
        PlanShapes/GeoJSON/GeoJsonPlanDocument.h
        PlanShapes/GeoJSON/GeoJsonPlanExporter.cc
        PlanShapes/GeoJSON/GeoJsonPlanExporter.h
        PlanShapes/GeoJSON/GeoJsonPlanImporter.cc
        PlanShapes/GeoJSON/GeoJsonPlanImporter.h

        # PlanShapes - GPX format
        PlanShapes/GPX/GpxPlanDocument.cc
        PlanShapes/GPX/GpxPlanDocument.h
        PlanShapes/GPX/GpxPlanExporter.cc
        PlanShapes/GPX/GpxPlanExporter.h
        PlanShapes/GPX/GpxPlanImporter.cc
        PlanShapes/GPX/GpxPlanImporter.h

        # PlanShapes - KML format
        PlanShapes/KML/KMLPlanDomDocument.cc
        PlanShapes/KML/KMLPlanDomDocument.h
        PlanShapes/KML/KmlPlanExporter.cc
        PlanShapes/KML/KmlPlanExporter.h
        PlanShapes/KML/KmlPlanImporter.cc
        PlanShapes/KML/KmlPlanImporter.h
        PlanShapes/KML/KmzPlanExporter.cc
        PlanShapes/KML/KmzPlanExporter.h
        PlanShapes/KML/KmzPlanImporter.cc
        PlanShapes/KML/KmzPlanImporter.h

        # PlanShapes - OpenAir format
        PlanShapes/OpenAir/OpenAirPlanImporter.cc
        PlanShapes/OpenAir/OpenAirPlanImporter.h

        # PlanShapes - SHP format
        PlanShapes/SHP/ShpPlanDocument.cc
        PlanShapes/SHP/ShpPlanDocument.h
        PlanShapes/SHP/ShpPlanExporter.cc
        PlanShapes/SHP/ShpPlanExporter.h
        PlanShapes/SHP/ShpPlanImporter.cc
        PlanShapes/SHP/ShpPlanImporter.h

        # PlanShapes - WKT format
        PlanShapes/WKT/WktPlanDocument.cc
        PlanShapes/WKT/WktPlanDocument.h
        PlanShapes/WKT/WktPlanExporter.cc
        PlanShapes/WKT/WktPlanExporter.h
        PlanShapes/WKT/WktPlanImporter.cc
        PlanShapes/WKT/WktPlanImporter.h

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

# ----------------------------------------------------------------------------
# Mission Definition JSON Resources
# ----------------------------------------------------------------------------
file(GLOB_RECURSE JSON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_mission_manager
    PREFIX "/json"
    FILES ${JSON_FILES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/CSV
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/GeoJSON
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/GPX
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/KML
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/OpenAir
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/SHP
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanShapes/WKT
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
