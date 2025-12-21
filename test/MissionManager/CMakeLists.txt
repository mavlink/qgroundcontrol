# ============================================================================
# Mission Manager Unit Tests
# Tests for mission planning, waypoints, and flight plan management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CameraCalcTest.cc CameraCalcTest.h
        CameraSectionTest.cc CameraSectionTest.h
        CorridorScanComplexItemTest.cc CorridorScanComplexItemTest.h
        FWLandingPatternTest.cc FWLandingPatternTest.h
        LandingComplexItemTest.cc LandingComplexItemTest.h
        MissionCommandTreeEditorTest.cc MissionCommandTreeEditorTest.h
        MissionCommandTreeTest.cc MissionCommandTreeTest.h
        MissionControllerManagerTest.cc MissionControllerManagerTest.h
        MissionControllerTest.cc MissionControllerTest.h
        MissionItemTest.cc MissionItemTest.h
        MissionManagerTest.cc MissionManagerTest.h
        MissionSettingsTest.cc MissionSettingsTest.h
        PlanMasterControllerTest.cc PlanMasterControllerTest.h
        QGCMapPolygonTest.cc QGCMapPolygonTest.h
        QGCMapPolylineTest.cc QGCMapPolylineTest.h
        SectionTest.cc SectionTest.h
        SimpleMissionItemTest.cc SimpleMissionItemTest.h
        SpeedSectionTest.cc SpeedSectionTest.h
        StructureScanComplexItemTest.cc StructureScanComplexItemTest.h
        SurveyComplexItemTest.cc SurveyComplexItemTest.h
        TransectStyleComplexItemTestBase.cc TransectStyleComplexItemTestBase.h
        TransectStyleComplexItemTest.cc TransectStyleComplexItemTest.h
        VisualMissionItemTest.cc VisualMissionItemTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "MissionManagerTest_res"
    PREFIX "/unittest"
    FILES
        MissionPlanner.waypoints
        MissionPlanner.waypoints.txt
        PolygonAreaTest.kml
        PolygonBadCoordinatesNode.kml
        PolygonBadXml.kml
        PolygonGood.kml
        PolygonMissingNode.kml
        SectionTest.plan
        UT-MavCmdInfoCommon.json
        UT-MavCmdInfoFixedWing.json
        UT-MavCmdInfoMultiRotor.json
        UT-MavCmdInfoRover.json
        UT-MavCmdInfoSub.json
        UT-MavCmdInfoVTOL.json
)

qt_add_resources(${CMAKE_PROJECT_NAME} "MissionManagerTest_res1"
    PREFIX "/qml"
    FILES
        MissionCommandTreeEditorTestWindow.qml
)

# TODO: Enable QML module for mission manager tests
# qt_add_library(MissionManagerTestModule STATIC)
#
# qt_add_qml_module(MissionManagerTestModule
#     URI QGroundControl.Test.MissionManager
#     VERSION 1.0
#     RESOURCE_PREFIX /unittest
#     QML_FILES
#         MissionCommandTreeEditorTestWindow.qml
#     NO_PLUGIN
# )
#
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE MissionManagerTestModule)
