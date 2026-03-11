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
