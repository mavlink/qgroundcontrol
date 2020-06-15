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
    property real   _cameraMinTriggerInterval:  _missionItem.cameraCalc.minTriggerInterval.rawValue
    property bool   _polygonDone:               false
    property string _doneAdjusting:             qsTr("Done")
    property var    _missionItem:               missionItem
    property bool   _presetsAvailable:          _missionItem.presetNames.length !== 0

    function polygonCaptureStarted() {
        _missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            _missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        _missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
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
            visible:        !_missionItem.surveyAreaPolygon.isValid || _missionItem.wizardMode

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
            }
        }

        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        !wizardColumn.visible

            TransectStyleComplexItemTabBar {
                id:             tabBar
                anchors.left:   parent.left
                anchors.right:  parent.right
            }

            // Grid tab
            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex === 0

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("WARNING: Photo interval is below minimum interval (%1 secs) supported by camera.").arg(_cameraMinTriggerInterval.toFixed(1))
                    wrapMode:       Text.WordWrap
                    color:          qgcPal.warningText
                    visible:        _missionItem.cameraShots > 0 && _cameraMinTriggerInterval !== 0 && _cameraMinTriggerInterval > _missionItem.timeBetweenShots
                }

                CameraCalcGrid {
                    cameraCalc:                     _missionItem.cameraCalc
                    vehicleFlightIsFrontal:         true
                    distanceToSurfaceLabel:         qsTr("Altitude")
                    distanceToSurfaceAltitudeMode:  _missionItem.followTerrain ?
                                                        QGroundControl.AltitudeModeAboveTerrain :
                                                        (_missionItem.cameraCalc.distanceToSurfaceRelative ? QGroundControl.AltitudeModeRelative : QGroundControl.AltitudeModeAbsolute)
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
                        fact:                   _missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              angleSlider.value = _missionItem.gridAngle.value
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
                        onValueChanged:         _missionItem.gridAngle.value = value
                        Component.onCompleted:  value = _missionItem.gridAngle.value
                        updateValueWhileDragging: true
                    }

                    QGCLabel {
                        text:       qsTr("Turnaround dist")
                    }
                    FactTextField {
                        fact:               _missionItem.turnAroundDistance
                        Layout.fillWidth:   true
                    }
                }

                QGCButton {
                    text:               qsTr("Rotate Entry Point")
                    onClicked:          _missionItem.rotateEntryPoint();
                }

                ColumnLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    visible:        transectsHeader.checked

                    QGCOptionsComboBox {
                        Layout.fillWidth: true

                        model: [
                            {
                                text:       qsTr("Hover and capture image"),
                                fact:       _missionItem.hoverAndCapture,
                                enabled:    !_missionItem.followTerrain,
                                visible:    _missionItem.hoverAndCaptureAllowed
                            },
                            {
                                text:       qsTr("Refly at 90 deg offset"),
                                fact:       _missionItem.refly90Degrees,
                                enabled:    !_missionItem.followTerrain,
                                visible:    true
                            },
                            {
                                text:       qsTr("Images in turnarounds"),
                                fact:       _missionItem.cameraTriggerInTurnAround,
                                enabled:    _missionItem.hoverAndCaptureAllowed ? !_missionItem.hoverAndCapture.rawValue : true,
                                visible:    true
                            },
                            {
                                text:       qsTr("Fly alternate transects"),
                                fact:       _missionItem.flyAlternateTransects,
                                enabled:    true,
                                visible:    _vehicle ? (_vehicle.fixedWing || _vehicle.vtol) : false
                            },
                            {
                                text:       qsTr("Relative altitude"),
                                enabled:    _missionItem.cameraCalc.isManualCamera && !_missionItem.followTerrain,
                                visible:    QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || (!_missionItem.cameraCalc.distanceToSurfaceRelative && !_missionItem.followTerrain),
                                checked:    _missionItem.cameraCalc.distanceToSurfaceRelative
                            }
                        ]

                        onItemClicked: {
                            if (index == 4) {
                                _missionItem.cameraCalc.distanceToSurfaceRelative = !_missionItem.cameraCalc.distanceToSurfaceRelative
                                console.log(_missionItem.cameraCalc.distanceToSurfaceRelative)
                            }
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

            // Camera Tab
            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex === 1

                CameraCalcCamera {
                    cameraCalc: _missionItem.cameraCalc
                }
            } // Camera Column

            // Terrain Tab
            TransectStyleComplexItemTerrainFollow {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                visible:        tabBar.currentIndex === 2
                missionItem:    _missionItem
            }

            // Presets Tab
            ColumnLayout {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex === 3

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("Presets")
                    wrapMode:           Text.WordWrap
                }

                QGCComboBox {
                    id:                 presetCombo
                    Layout.fillWidth:   true
                    model:              _missionItem.presetNames
                }

                RowLayout {
                    Layout.fillWidth:   true

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Apply Preset")
                        enabled:            _missionItem.presetNames.length != 0
                        onClicked:          _missionItem.loadPreset(presetCombo.textAt(presetCombo.currentIndex))
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Delete Preset")
                        enabled:            _missionItem.presetNames.length != 0
                        onClicked:          mainWindow.showComponentDialog(deletePresetMessage, qsTr("Delete Preset"), mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)

                        Component {
                            id: deletePresetMessage
                            QGCViewMessage {
                                message: qsTr("Are you sure you want to delete '%1' preset?").arg(presetName)
                                property string presetName: presetCombo.textAt(presetCombo.currentIndex)
                                function accept() {
                                    _missionItem.deletePreset(presetName)
                                    hideDialog()
                                }
                            }
                        }
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
                        fact:                   _missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              presetsAngleSlider.value = _missionItem.gridAngle.value
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
                        onValueChanged:         _missionItem.gridAngle.value = value
                        Component.onCompleted:  value = _missionItem.gridAngle.value
                        updateValueWhileDragging: true
                    }

                    QGCButton {
                        Layout.columnSpan:  2
                        Layout.fillWidth:   true
                        text:               qsTr("Rotate Entry Point")
                        onClicked:          _missionItem.rotateEntryPoint();
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
                        _missionItem.savePreset(presetNameField.text)
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
            _missionItem.surveyAreaPolygon.loadKMLOrSHPFile(file)
            _missionItem.resetState = false
            //editorMap.mapFitFunctions.fitMapViewportTo_missionItems()
            close()
        }
    }
} // Rectangle
