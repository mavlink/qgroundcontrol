import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Extras   1.4
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

Rectangle {
    id:         _root
    height:     visible ? (editorColumn.height + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real   _margin:                    ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:                ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:                   QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property real   _cameraMinTriggerInterval:  missionItem.cameraCalc.minTriggerInterval.rawValue
    property bool   _polygonDone:               false
    property string _doneAdjusting:             qsTr("Done Adjusting")
    property bool   _presetsAvailable:          missionItem.presetNames.length !== 0

    function polygonCaptureStarted() {
        missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right

        ColumnLayout {
            id:             wizardColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        !missionItem.surveyAreaPolygon.isValid || missionItem.wizardMode

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:             _margin
                visible:            !_polygonDone

                QGCLabel {
                    Layout.fillWidth:       true
                    wrapMode:               Text.WordWrap
                    horizontalAlignment:    Text.AlignHCenter
                    text:                   qsTr("Use the Polygon Tools to create the polygon which outlines your survey area.")
                }

                QGCButton {
                    text:               qsTr("Done With Polygon")
                    Layout.fillWidth:   true
                    enabled:            missionItem.surveyAreaPolygon.isValid
                    onClicked: {
                        if (!_presetsAvailable) {
                            missionItem.wizardMode = false
                            editorRoot.selectNextNotReadyItem()
                        }
                        _polygonDone = true
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            _margin
                visible:            _polygonDone

                QGCLabel {
                    Layout.fillWidth:       true
                    wrapMode:               Text.WordWrap
                    horizontalAlignment:    Text.AlignHCenter
                    text:                   qsTr("Apply a Preset or click %1 for manual setup.").arg(_doneAdjusting)
                }

                QGCComboBox {
                    id:                 wizardPresetCombo
                    Layout.fillWidth:   true
                    model:              missionItem.presetNames
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Apply Preset")
                    enabled:            missionItem.presetNames.length != 0
                    onClicked:          missionItem.loadPreset(wizardPresetCombo.textAt(wizardPresetCombo.currentIndex))
                }

                SectionHeader {
                    id:                 wizardPresectsTransectsHeader
                    Layout.fillWidth:   true
                    text:               qsTr("Transects")
                }

                GridLayout {
                    Layout.fillWidth:   true
                    columnSpacing:      _margin
                    rowSpacing:         _margin
                    columns:            2
                    visible:            wizardPresectsTransectsHeader.checked

                    QGCLabel { text: qsTr("Angle") }
                    FactTextField {
                        fact:                   missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              wizardPresetsAngleSlider.value = missionItem.gridAngle.value
                    }

                    QGCSlider {
                        id:                     wizardPresetsAngleSlider
                        minimumValue:           0
                        maximumValue:           359
                        stepSize:               1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        onValueChanged:         missionItem.gridAngle.value = value
                        Component.onCompleted:  value = missionItem.gridAngle.value
                        updateValueWhileDragging: true
                    }

                    QGCButton {
                        Layout.columnSpan:  2
                        Layout.fillWidth:   true
                        text:               qsTr("Rotate Entry Point")
                        onClicked:          missionItem.rotateEntryPoint();
                    }
                }

                Item { height: ScreenTools.defaultFontPixelHeight; width: 1 }

                QGCButton {
                    text:               _doneAdjusting
                    Layout.fillWidth:   true
                    enabled:            missionItem.surveyAreaPolygon.isValid
                    onClicked: {
                        missionItem.wizardMode = false
                        editorRoot.selectNextNotReadyItem()
                    }
                }
            }
        }

        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        !wizardColumn.visible

            QGCTabBar {
                id:             tabBar
                anchors.left:   parent.left
                anchors.right:  parent.right

                Component.onCompleted: currentIndex = QGroundControl.settingsManager.planViewSettings.displayPresetsTabFirst.rawValue ? 2 : 0

                QGCTabButton { text: qsTr("Grid") }
                QGCTabButton { text: qsTr("Camera") }
                QGCTabButton { text: qsTr("Presets") }
            }

            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex == 0

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("WARNING: Photo interval is below minimum interval (%1 secs) supported by camera.").arg(_cameraMinTriggerInterval.toFixed(1))
                    wrapMode:       Text.WordWrap
                    color:          qgcPal.warningText
                    visible:        missionItem.cameraShots > 0 && _cameraMinTriggerInterval !== 0 && _cameraMinTriggerInterval > missionItem.timeBetweenShots
                }

                CameraCalcGrid {
                    cameraCalc:                     missionItem.cameraCalc
                    vehicleFlightIsFrontal:         true
                    distanceToSurfaceLabel:         qsTr("Altitude")
                    distanceToSurfaceAltitudeMode:  missionItem.followTerrain ?
                                                        QGroundControl.AltitudeModeAboveTerrain :
                                                        missionItem.cameraCalc.distanceToSurfaceRelative
                    frontalDistanceLabel:           qsTr("Trigger Dist")
                    sideDistanceLabel:              qsTr("Spacing")
                }

                SectionHeader {
                    id:             transectsHeader
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Transects")
                }

                GridLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    columnSpacing:  _margin
                    rowSpacing:     _margin
                    columns:        2
                    visible:        transectsHeader.checked

                    QGCLabel { text: qsTr("Angle") }
                    FactTextField {
                        fact:                   missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              angleSlider.value = missionItem.gridAngle.value
                    }

                    QGCSlider {
                        id:                     angleSlider
                        minimumValue:           0
                        maximumValue:           359
                        stepSize:               1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        onValueChanged:         missionItem.gridAngle.value = value
                        Component.onCompleted:  value = missionItem.gridAngle.value
                        updateValueWhileDragging: true
                    }

                    QGCLabel {
                        text:       qsTr("Turnaround dist")
                    }
                    FactTextField {
                        fact:               missionItem.turnAroundDistance
                        Layout.fillWidth:   true
                    }
                }

                QGCButton {
                    text:               qsTr("Rotate Entry Point")
                    onClicked:          missionItem.rotateEntryPoint();
                }

                ColumnLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    visible:        transectsHeader.checked

                    /*
              Temporarily removed due to bug https://github.com/mavlink/qgroundcontrol/issues/7005
            FactCheckBox {
                text:       qsTr("Split concave polygons")
                fact:       _splitConcave
                visible:    _splitConcave.visible
                property Fact _splitConcave: missionItem.splitConcavePolygons
            }
            */

                    QGCOptionsComboBox {
                        Layout.fillWidth: true

                        model: [
                            {
                                text:       qsTr("Hover and capture image"),
                                fact:       missionItem.hoverAndCapture,
                                enabled:    !missionItem.followTerrain,
                                visible:    missionItem.hoverAndCaptureAllowed
                            },
                            {
                                text:       qsTr("Refly at 90 deg offset"),
                                fact:       missionItem.refly90Degrees,
                                enabled:    !missionItem.followTerrain,
                                visible:    true
                            },
                            {
                                text:       qsTr("Images in turnarounds"),
                                fact:       missionItem.cameraTriggerInTurnAround,
                                enabled:    missionItem.hoverAndCaptureAllowed ? !missionItem.hoverAndCapture.rawValue : true,
                                visible:    true
                            },
                            {
                                text:       qsTr("Fly alternate transects"),
                                fact:       missionItem.flyAlternateTransects,
                                enabled:    true,
                                visible:    _vehicle ? (_vehicle.fixedWing || _vehicle.vtol) : false
                            },
                            {
                                text:       qsTr("Relative altitude"),
                                enabled:    missionItem.cameraCalc.isManualCamera && !missionItem.followTerrain,
                                visible:    QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || (!missionItem.cameraCalc.distanceToSurfaceRelative && !missionItem.followTerrain),
                                checked:    missionItem.cameraCalc.distanceToSurfaceRelative
                            }
                        ]

                        onItemClicked: {
                            if (index == 4) {
                                missionItem.cameraCalc.distanceToSurfaceRelative = !missionItem.cameraCalc.distanceToSurfaceRelative
                                console.log(missionItem.cameraCalc.distanceToSurfaceRelative)
                            }
                        }
                    }
                }

                SectionHeader {
                    id:             terrainHeader
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Terrain")
                    checked:        missionItem.followTerrain
                }

                ColumnLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    visible:        terrainHeader.checked


                    QGCCheckBox {
                        id:         followsTerrainCheckBox
                        text:       qsTr("Vehicle follows terrain")
                        checked:    missionItem.followTerrain
                        onClicked:  missionItem.followTerrain = checked
                    }

                    GridLayout {
                        Layout.fillWidth:   true
                        columnSpacing:      _margin
                        rowSpacing:         _margin
                        columns:            2
                        visible:            followsTerrainCheckBox.checked

                        QGCLabel { text: qsTr("Tolerance") }
                        FactTextField {
                            fact:               missionItem.terrainAdjustTolerance
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("Max Climb Rate") }
                        FactTextField {
                            fact:               missionItem.terrainAdjustMaxClimbRate
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("Max Descent Rate") }
                        FactTextField {
                            fact:               missionItem.terrainAdjustMaxDescentRate
                            Layout.fillWidth:   true
                        }
                    }
                }

                SectionHeader {
                    id:             statsHeader
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Statistics")
                }

                TransectStyleComplexItemStats {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    visible:        statsHeader.checked
                }
            } // Grid Column

            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex == 1

                CameraCalcCamera {
                    cameraCalc:                     missionItem.cameraCalc
                    vehicleFlightIsFrontal:         true
                    distanceToSurfaceLabel:         qsTr("Altitude")
                    distanceToSurfaceAltitudeMode:  missionItem.followTerrain ?
                                                        QGroundControl.AltitudeModeAboveTerrain :
                                                        missionItem.cameraCalc.distanceToSurfaceRelative
                    frontalDistanceLabel:           qsTr("Trigger Dist")
                    sideDistanceLabel:              qsTr("Spacing")
                }
            } // Camera Column

            ColumnLayout {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex == 2

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("Presets")
                    wrapMode:           Text.WordWrap
                }

                QGCComboBox {
                    id:                 presetCombo
                    Layout.fillWidth:   true
                    model:              missionItem.presetNames
                }

                RowLayout {
                    Layout.fillWidth:   true

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Apply Preset")
                        enabled:            missionItem.presetNames.length != 0
                        onClicked:          missionItem.loadPreset(presetCombo.textAt(presetCombo.currentIndex))
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Delete Preset")
                        enabled:            missionItem.presetNames.length != 0
                        onClicked:          missionItem.deletePreset(presetCombo.textAt(presetCombo.currentIndex))
                    }

                }

                Item { height: ScreenTools.defaultFontPixelHeight; width: 1 }

                QGCButton {
                    Layout.alignment:   Qt.AlignCenter
                    Layout.fillWidth:   true
                    text:               qsTr("Save Settings As New Preset")
                    onClicked:          mainWindow.showComponentDialog(savePresetDialog, qsTr("Save Preset"), mainWindow.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
                }

                SectionHeader {
                    id:                 presectsTransectsHeader
                    Layout.fillWidth:   true
                    text:               qsTr("Transects")
                }

                GridLayout {
                    Layout.fillWidth:   true
                    columnSpacing:      _margin
                    rowSpacing:         _margin
                    columns:            2
                    visible:            presectsTransectsHeader.checked

                    QGCLabel { text: qsTr("Angle") }
                    FactTextField {
                        fact:                   missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              presetsAngleSlider.value = missionItem.gridAngle.value
                    }

                    QGCSlider {
                        id:                     presetsAngleSlider
                        minimumValue:           0
                        maximumValue:           359
                        stepSize:               1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        onValueChanged:         missionItem.gridAngle.value = value
                        Component.onCompleted:  value = missionItem.gridAngle.value
                        updateValueWhileDragging: true
                    }

                    QGCButton {
                        Layout.columnSpan:  2
                        Layout.fillWidth:   true
                        text:               qsTr("Rotate Entry Point")
                        onClicked:          missionItem.rotateEntryPoint();
                    }
                }

                SectionHeader {
                    id:                 presetsStatsHeader
                    Layout.fillWidth:   true
                    text:               qsTr("Statistics")
                }

                TransectStyleComplexItemStats {
                    Layout.fillWidth:   true
                    visible:            presetsStatsHeader.checked
                }
            } // Main editing column
        } // Top level  Column

        Component {
            id: savePresetDialog

            QGCViewDialog {
                function accept() {
                    if (presetNameField.text != "") {
                        missionItem.savePreset(presetNameField.text)
                        hideDialog()
                    }
                }

                ColumnLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        ScreenTools.defaultFontPixelHeight

                    QGCLabel {
                        Layout.fillWidth:   true
                        text:               qsTr("Save the current settings as a named preset.")
                        wrapMode:           Text.WordWrap
                    }

                    QGCLabel {
                        text: qsTr("Preset Name")
                    }

                    QGCTextField {
                        id:                 presetNameField
                        Layout.fillWidth:   true
                    }
                }
            }
        }
    }

    KMLOrSHPFileDialog {
        id:             kmlOrSHPLoadDialog
        title:          qsTr("Select Polygon File")
        selectExisting: true

        onAcceptedForLoad: {
            missionItem.surveyAreaPolygon.loadKMLOrSHPFile(file)
            missionItem.resetState = false
            //editorMap.mapFitFunctions.fitMapViewportToMissionItems()
            close()
        }
    }
} // Rectangle
