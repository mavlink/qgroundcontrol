set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraCalcTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraCalcTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraSectionTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraSectionTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CorridorScanComplexItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CorridorScanComplexItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FWLandingPatternTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FWLandingPatternTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LandingComplexItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LandingComplexItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandTreeEditorTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandTreeEditorTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandTreeTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionCommandTreeTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionControllerManagerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionControllerManagerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionControllerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionControllerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionManagerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionManagerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionSettingsTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MissionSettingsTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanMasterControllerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanMasterControllerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolygonTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolygonTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolylineTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolylineTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SectionTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SectionTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleMissionItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleMissionItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SpeedSectionTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SpeedSectionTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StructureScanComplexItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/StructureScanComplexItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SurveyComplexItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SurveyComplexItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransectStyleComplexItemTestBase.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TransectStyleComplexItemTestBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransectStyleComplexItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TransectStyleComplexItemTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VisualMissionItemTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VisualMissionItemTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# qt_add_qml_module(${CMAKE_PROJECT_NAME}
#     URI missionmanagertest
#     VERSION 1.0
#     QML_FILES
#         MissionCommandTreeEditorTestWindow.qml
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )
