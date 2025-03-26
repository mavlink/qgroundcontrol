find_package(Qt6 REQUIRED COMPONENTS Core Positioning Qml Test)

target_sources(qgctest
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

target_link_libraries(qgctest
    PRIVATE
        Qt6::Test
    PUBLIC
        Qt6::Positioning
)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(qgctest
#     URI missionmanagertest
#     VERSION 1.0
#     QML_FILES
#         MissionCommandTreeEditorTestWindow.qml
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )
