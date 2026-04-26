# ============================================================================
# Plan View Module
# Mission planning view, editors, and map visuals
# ============================================================================

qt_add_library(PlanViewModule STATIC)

qt_add_qml_module(PlanViewModule
    URI QGroundControl.PlanView
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        CameraCalcCamera.qml
        CameraCalcGrid.qml
        CameraSection.qml
        CorridorScanEditor.qml
        CorridorScanMapVisual.qml
        FWLandingPatternEditor.qml
        FWLandingPatternMapVisual.qml
        GeoFenceEditor.qml
        GeoFenceMapVisuals.qml
        MissionDefaultsEditor.qml
        MissionItemEditor.qml
        MissionItemIndexLabel.qml
        MissionItemMapVisual.qml
        MissionItemMapVisualBase.qml
        HomePositionMapVisual.qml
        MissionItemStatus.qml
        MissionSettingsEditor.qml
        MissionStats.qml
        PlanEditToolbar.qml
        PlanInfoEditor.qml
        PlanToolBarIndicators.qml
        PlanTreeView.qml
        PlanView.qml
        PlanViewRightPanel.qml
        RallyPointEditorHeader.qml
        RallyPointItemEditor.qml
        RallyPointMapVisuals.qml
        SimpleItemEditor.qml
        SimpleItemMapVisual.qml
        StructureScanEditor.qml
        StructureScanMapVisual.qml
        SurveyItemEditor.qml
        SurveyMapVisual.qml
        TakeoffItemMapVisual.qml
        TerrainStatus.qml
        TransectStyleComplexItemEditor.qml
        TransectStyleComplexItemStats.qml
        TransectStyleComplexItemTabBar.qml
        TransectStyleComplexItemTerrainFollow.qml
        TransectStyleMapVisuals.qml
        TransformEditor.qml
        VTOLLandingPatternEditor.qml
        VTOLLandingPatternMapVisual.qml
    NO_PLUGIN
)

# Files whose runtime alias differs from their on-disk filename.
set_source_files_properties("${CMAKE_SOURCE_DIR}/resources/AdvancedSliders.svg"
    PROPERTIES QT_RESOURCE_ALIAS "PlanSimpleItemAdvanced.svg")
set_source_files_properties("${CMAKE_SOURCE_DIR}/resources/gear-black.svg"
    PROPERTIES QT_RESOURCE_ALIAS "PlanSimpleItemBasic.svg")
set_source_files_properties("${CMAKE_SOURCE_DIR}/resources/camera.svg"
    PROPERTIES QT_RESOURCE_ALIAS "PlanSimpleItemCamera.svg")

qt_add_resources(${CMAKE_PROJECT_NAME} planview_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/AdvancedSliders.svg"
        "${CMAKE_SOURCE_DIR}/resources/camera.svg"
        "${CMAKE_SOURCE_DIR}/resources/chevron-double-left.svg"
        "${CMAKE_SOURCE_DIR}/resources/DropArrow.svg"
        "${CMAKE_SOURCE_DIR}/resources/gear-black.svg"
        "${CMAKE_SOURCE_DIR}/resources/RallyPoint.svg"
        "${CMAKE_SOURCE_DIR}/resources/sliders.svg"
        "${CMAKE_SOURCE_DIR}/resources/SaveToDisk.svg"
        "${CMAKE_SOURCE_DIR}/resources/terrain.svg"
        "${CMAKE_SOURCE_DIR}/resources/UploadToVehicle.svg"
        "${CMAKE_SOURCE_DIR}/resources/waypoint.svg"
)

qt_add_resources(${CMAKE_PROJECT_NAME} planview_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/PatternCamera.png"
        "${CMAKE_SOURCE_DIR}/resources/PatternGrid.png"
        "${CMAKE_SOURCE_DIR}/resources/PatternPresets.png"
        "${CMAKE_SOURCE_DIR}/resources/PatternTerrain.png"
)
