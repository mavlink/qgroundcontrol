# ============================================================================
# QML Controls Module
# Reusable QML UI controls and controllers
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FactValueGrid.cc
        FactValueGrid.h
        FlightPathSegment.cc
        FlightPathSegment.h
        HorizontalFactValueGrid.cc
        HorizontalFactValueGrid.h
        InstrumentValueData.cc
        InstrumentValueData.h
        MavlinkAction.cc
        MavlinkAction.h
        MavlinkActionManager.cc
        MavlinkActionManager.h
        ObjectItemModelBase.cc
        ObjectItemModelBase.h
        ObjectListModelBase.cc
        ObjectListModelBase.h
        ParameterEditorController.cc
        ParameterEditorController.h
        QGCFenceCircle.cc
        QGCFenceCircle.h
        QGCFencePolygon.cc
        QGCFencePolygon.h
        QGCFileDialogController.cc
        QGCFileDialogController.h
        QGCGeoBoundingCube.cc
        QGCGeoBoundingCube.h
        QGCImageProvider.cc
        QGCImageProvider.h
        QGCMapCircle.cc
        QGCMapCircle.h
        QGCMapPalette.cc
        QGCMapPalette.h
        QGCMapPolygon.cc
        QGCMapPolygon.h
        QGCMapPolyline.cc
        QGCMapPolyline.h
        QGCPalette.cc
        QGCPalette.h
        QGCQGeoCoordinate.cc
        QGCQGeoCoordinate.h
        QGroundControlQmlGlobal.cc
        QGroundControlQmlGlobal.h
        QmlObjectListModel.cc
        QmlObjectListModel.h
        QmlObjectTreeModel.cc
        QmlObjectTreeModel.h
        QmlUnitsConversion.h
        RCChannelMonitorController.cc
        RCChannelMonitorController.h
        RCToParamDialogController.cc
        RCToParamDialogController.h
        ScreenToolsController.cc
        ScreenToolsController.h
        TerrainProfile.cc
        TerrainProfile.h
        ToolStripAction.cc
        ToolStripAction.h
        ToolStripActionList.cc
        ToolStripActionList.h
        TransformPositionController.cc
        TransformPositionController.h
        ServoOutputMonitorController.cc
        ServoOutputMonitorController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# QML Controls Module
# ----------------------------------------------------------------------------
qt_add_library(QGroundControlControlsModule STATIC)

set_source_files_properties(ScreenTools.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(QGroundControlControlsModule
    URI QGroundControl.Controls
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        AltFrameCombo.qml
        AltFrameDialog.qml
        AppSettings.qml
        APMSubMotorDisplay.qml
        AppLogging.qml
        AutotuneUI.qml
        AxisMonitor.qml
        ClickableColor.qml
        ConfigButton.qml
        ConfigSection.qml
        DeadMouseArea.qml
        DropButton.qml
        DropPanel.qml
        EditPositionDialog.qml
        ExclusiveGroupItem.qml
        FactSlider.qml
        FileButton.qml
        FlightModeMenu.qml
        HeightIndicator.qml
        HorizontalFactValueGrid.qml
        IndicatorButton.qml
        InstrumentValueEditDialog.qml
        InstrumentValueLabel.qml
        InstrumentValueValue.qml
        JoystickThumbPad.qml
        KMLOrSHPFileDialog.qml
        LabelledButton.qml
        LabelledComboBox.qml
        LabelledLabel.qml
        LogReplayStatusBar.qml
        MainWindowSavedState.qml
        MissionCommandDialog.qml
        MvPanelPage.qml
        OfflineMapButton.qml
        ParameterDiffDialog.qml
        ParameterEditor.qml
        ParameterEditorDialog.qml
        PIDTuning.qml
        PipState.qml
        PipView.qml
        PreFlightCheckButton.qml
        PreFlightCheckGroup.qml
        PreFlightCheckModel.qml
        QGCButton.qml
        QGCCheckBox.qml
        QGCCheckBoxSlider.qml
        QGCColoredImage.qml
        QGCColumnButton.qml
        QGCComboBox.qml
        QGCDelayButton.qml
        QGCDynamicObjectManager.qml
        QGCFileDialog.qml
        QGCFlickable.qml
        QGCFlickableScrollIndicator.qml
        QGCGroupBox.qml
        QGCLabel.qml
        QGCListView.qml
        QGCMapLabel.qml
        QGCMarqueeLabel.qml
        QGCMenu.qml
        QGCMenuItem.qml
        QGCMenuSeparator.qml
        QGCMouseArea.qml
        QGCMovableItem.qml
        QGCOptionsComboBox.qml
        QGCPageIndicator.qml
        QGCPopupDialog.qml
        QGCPopupDialogFactory.qml
        QGCRadioButton.qml
        QGCRoundButton.qml
        QGCSimpleMessageDialog.qml
        QGCSlider.qml
        QGCSwipeView.qml
        QGCTabBar.qml
        QGCTabButton.qml
        QGCTextField.qml
        QGCToolBarButton.qml
        QGCToolInsets.qml
        RCChannelMonitor.qml
        RCToParamDialog.qml
        SectionHeader.qml
        SelectableControl.qml
        SettingsButton.qml
        SettingsGroupLayout.qml
        SetupPage.qml
        SigningKeyManager.qml
        SliderSwitch.qml
        SubMenuButton.qml
        TimedProgressTracker.qml
        ToolIndicatorPage.qml
        ToolStrip.qml
        ToolStripDropPanel.qml
        ToolStripHoverButton.qml
        ValueSlider.qml
        VehicleRotationCal.qml
        VehicleSummaryRow.qml

# NOTE: ScreenTools.qml is declared here because CMake's
# set_source_files_properties() has directory scope. The QT_QML_SINGLETON_TYPE
# property must be visible in the same CMakeLists.txt that calls
# qt_add_qml_module(). On Windows this cross-directory property propagation
# fails, so this file is kept in this module until a better solution is available.

        ScreenTools.qml
    NO_PLUGIN
)

# ----------------------------------------------------------------------------
# Control Configuration Resources
# ----------------------------------------------------------------------------
file(GLOB_RECURSE JSON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_controls
    PREFIX "/json"
    FILES ${JSON_FILES}
)
