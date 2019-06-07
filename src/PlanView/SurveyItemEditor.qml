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
    property string _currentPreset:             missionItem.currentPreset
    property bool   _usingPreset:               _currentPreset !== ""

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
        spacing:            _margin

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            text:           qsTr("WARNING: Photo interval is below minimum interval (%1 secs) supported by camera.").arg(_cameraMinTriggerInterval.toFixed(1))
            wrapMode:       Text.WordWrap
            color:          qgcPal.warningText
            visible:        missionItem.cameraShots > 0 && _cameraMinTriggerInterval !== 0 && _cameraMinTriggerInterval > missionItem.timeBetweenShots
        }

        QGCLabel {
            text: qsTr("Presets")
        }

        QGCComboBox {
            id:                 presetCombo
            anchors.left:       parent.left
            anchors.right:      parent.right
            model:              _presetList

            property var _presetList:   []

            readonly property int _indexCustom:         0
            readonly property int _indexCreate:         1
            readonly property int _indexDelete:         2
            readonly property int _indexLabel:          3
            readonly property int _indexFirstPreset:    4

            Component.onCompleted: _updateList()

            onActivated: {
                if (index == _indexCustom) {
                    missionItem.clearCurrentPreset()
                } else if (index == _indexCreate) {
                    mainWindow.showComponentDialog(savePresetDialog, qsTr("Save Preset"), mainWindow.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
                } else if (index == _indexDelete) {
                    if (missionItem.builtInPreset) {
                        mainWindow.showMessage(qsTr("Delete Preset"), qsTr("This preset cannot be deleted."))
                    } else {
                        missionItem.deleteCurrentPreset()
                    }
                } else if (index >= _indexFirstPreset) {
                    missionItem.loadPreset(textAt(index))
                } else {
                    _selectCurrentPreset()
                }
            }

            Connections {
                target: missionItem

                onPresetNamesChanged:   presetCombo._updateList()
                onCurrentPresetChanged: presetCombo._selectCurrentPreset()
            }

            // There is some major strangeness going on with programatically changing the index of a combo box in this scenario.
            // If you just set currentIndex directly it will just change back 1o -1 magically. Has something to do with resetting
            // model on the fly I think. But not sure. To work around I delay the currentIndex changes to let things unwind.
            Timer {
                id:         delayedIndexChangeTimer
                interval:   10

                property int newIndex

                onTriggered:  presetCombo.currentIndex = newIndex

            }

            function delayedIndexChange(index) {
                delayedIndexChangeTimer.newIndex = index
                delayedIndexChangeTimer.start()
            }

            function _updateList() {
                _presetList = []
                _presetList.push(qsTr("Custom (specify all settings)"))
                _presetList.push(qsTr("Save Settings As Preset"))
                _presetList.push(qsTr("Delete Current Preset"))
                if (missionItem.presetNames.length !== 0) {
                    _presetList.push(qsTr("Presets:"))
                }

                for (var i=0; i<missionItem.presetNames.length; i++) {
                    _presetList.push(missionItem.presetNames[i])
                }
                model = _presetList
                _selectCurrentPreset()
            }

            function _selectCurrentPreset() {
                if (_usingPreset) {
                    var newIndex = find(_currentPreset)
                    if (newIndex !== -1) {
                        delayedIndexChange(newIndex)
                        return
                    }
                }
                delayedIndexChange(presetCombo._indexCustom)
            }
        }

        CameraCalc {
            cameraCalc:                     missionItem.cameraCalc
            vehicleFlightIsFrontal:         true
            distanceToSurfaceLabel:         qsTr("Altitude")
            distanceToSurfaceAltitudeMode:  missionItem.followTerrain ? QGroundControl.AltitudeModeAboveTerrain : QGroundControl.AltitudeModeRelative
            frontalDistanceLabel:           qsTr("Trigger Dist")
            sideDistanceLabel:              qsTr("Spacing")
            usingPreset:                    _usingPreset
            cameraSpecifiedInPreset:        missionItem.cameraInPreset
        }

        SectionHeader {
            id:     transectsHeader
            text:   qsTr("Transects")
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
                visible:    !_usingPreset
            }
            FactTextField {
                fact:               missionItem.turnAroundDistance
                Layout.fillWidth:   true
                visible:            !_usingPreset
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
            visible:        transectsHeader.checked && !_usingPreset

            /*
              Temporarily removed due to bug https://github.com/mavlink/qgroundcontrol/issues/7005
            FactCheckBox {
                text:       qsTr("Split concave polygons")
                fact:       _splitConcave
                visible:    _splitConcave.visible
                property Fact _splitConcave: missionItem.splitConcavePolygons
            }
            */

            FactCheckBox {
                text:               qsTr("Hover and capture image")
                fact:               missionItem.hoverAndCapture
                visible:            missionItem.hoverAndCaptureAllowed
                enabled:            !missionItem.followTerrain
                onClicked: {
                    if (checked) {
                        missionItem.cameraTriggerInTurnAround.rawValue = false
                    }
                }
            }

            FactCheckBox {
                text:               qsTr("Refly at 90 deg offset")
                fact:               missionItem.refly90Degrees
                enabled:            !missionItem.followTerrain
            }

            FactCheckBox {
                text:               qsTr("Images in turnarounds")
                fact:               missionItem.cameraTriggerInTurnAround
                enabled:            missionItem.hoverAndCaptureAllowed ? !missionItem.hoverAndCapture.rawValue : true
            }

            FactCheckBox {
                text:               qsTr("Fly alternate transects")
                fact:               missionItem.flyAlternateTransects
                visible:            _vehicle ? (_vehicle.fixedWing || _vehicle.vtol) : false
            }

            QGCCheckBox {
                id:                 relAlt
                Layout.alignment:   Qt.AlignLeft
                text:               qsTr("Relative altitude")
                checked:            missionItem.cameraCalc.distanceToSurfaceRelative
                enabled:            missionItem.cameraCalc.isManualCamera && !missionItem.followTerrain
                visible:            QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || (!missionItem.cameraCalc.distanceToSurfaceRelative && !missionItem.followTerrain)
                onClicked:          missionItem.cameraCalc.distanceToSurfaceRelative = checked

                Connections {
                    target: missionItem.cameraCalc
                    onDistanceToSurfaceRelativeChanged: relAlt.checked = missionItem.cameraCalc.distanceToSurfaceRelative
                }
            }
        }

        SectionHeader {
            id:         terrainHeader
            text:       qsTr("Terrain")
            checked:    missionItem.followTerrain
            visible:    !_usingPreset
        }

        ColumnLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        terrainHeader.checked && !_usingPreset


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
            id:     statsHeader
            text:   qsTr("Statistics")
        }

        TransectStyleComplexItemStats { }
    } // Column

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
                    text:               _currentPreset
                }

                QGCCheckBox {
                    text:       qsTr("Save Camera In Preset")
                    checked:    missionItem.cameraInPreset
                    onClicked:  missionItem.cameraInPreset = checked
                }
            }
        }
    }
} // Rectangle
