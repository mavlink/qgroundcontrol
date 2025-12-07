import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl


import QGroundControl.Controls
import QGroundControl.FactControls


// Editor for Simple mission items
Rectangle {
    width:  availableWidth
    height: editorColumn.height //+ (_margin * 2)
    color:  "transparent"
    radius: _radius

    property bool _specifiesAltitude:       missionItem.specifiesAltitude
    property real _margin:                  ScreenTools.defaultFontPixelHeight / 2
    property real _altRectMargin:           ScreenTools.defaultFontPixelWidth / 2
    property var  _controllerVehicle:       missionItem.masterController.controllerVehicle
    property int  _globalAltMode:           missionItem.masterController.missionController.globalAltitudeMode
    property bool _globalAltModeIsMixed:    _globalAltMode == QGroundControl.AltitudeModeMixed
    property real _radius:                  ScreenTools.defaultFontPixelWidth / 2
    property real _fieldSpacing:            ScreenTools.defaultFontPixelHeight / 2

    function updateAltitudeModeText() {
        if (missionItem.altitudeMode === QGroundControl.AltitudeModeRelative) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeRelative)
        } else if (missionItem.altitudeMode === QGroundControl.AltitudeModeAbsolute) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAbsolute)
        } else if (missionItem.altitudeMode === QGroundControl.AltitudeModeCalcAboveTerrain) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeCalcAboveTerrain)
        } else if (missionItem.altitudeMode === QGroundControl.AltitudeModeTerrainFrame) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeTerrainFrame)
        } else {
            altModeLabel.text = qsTr("Internal Error")
        }
    }

    Component.onCompleted: updateAltitudeModeText()

    Connections {
        target:                 missionItem
        function onAltitudeModeChanged() { updateAltitudeModeText() }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    Component { id: altModeDialogComponent; AltModeDialog { } }

    Column {
        id:         editorColumn
        width:      parent.width
        spacing:    _margin

        // Takeoff item
        ColumnLayout {
            width:      parent.width
            spacing:    _margin
            visible:    missionItem.isTakeoffItem && missionItem.wizardMode // Hack special case for takeoff item

            QGCLabel {
                text:               qsTr("Move '%1' %2 to the %3 location. %4")
                .arg(_controllerVehicle.vtol ? qsTr("T") : qsTr("T"))
                .arg(_controllerVehicle.vtol ? qsTr("Transition Direction") : qsTr("Takeoff"))
                .arg(_controllerVehicle.vtol ? qsTr("desired") : qsTr("climbout"))
                .arg(_controllerVehicle.vtol ? (qsTr("Ensure distance from launch to transition direction is far enough to complete transition.")) : "")
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                visible:            !initialClickLabel.visible
            }

            QGCLabel {
                text:               qsTr("Ensure clear of obstacles and into the wind.")
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                visible:            !initialClickLabel.visible
            }

            QGCButton {
                text:               qsTr("Done")
                Layout.fillWidth:   true
                visible:            !initialClickLabel.visible
                onClicked: {
                    missionItem.wizardMode = false
                }
            }

            QGCLabel {
                id:                 initialClickLabel
                text:               missionItem.launchTakeoffAtSameLocation ?
                                        qsTr("Click in map to set planned Takeoff location.") :
                                        qsTr("Click in map to set planned Launch location.")
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                visible:            missionItem.isTakeoffItem && !missionItem.launchCoordinate.isValid
            }
        }

        Column {
            width:      parent.width
            spacing:    _fieldSpacing
            visible:    !missionItem.wizardMode

            ColumnLayout {
                width:      parent.width
                spacing:    0
                visible:    _specifiesAltitude

                QGCLabel {
                    Layout.fillWidth:   true
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               qsTr("Altitude below specifies the approximate altitude of the ground. Normally 0 for landing back at original launch location.")
                    visible:            missionItem.isLandCommand
                }

                MouseArea {
                    Layout.preferredWidth:  childrenRect.width
                    Layout.preferredHeight: childrenRect.height

                    onClicked: {
                        if (_globalAltModeIsMixed) {
                            var removeModes = []
                            var updateFunction = function(altMode){ missionItem.altitudeMode = altMode }
                            if (!_controllerVehicle.supportsTerrainFrame) {
                                removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
                            }
                            if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude && missionItem.altitudeMode !== QGroundControl.AltitudeModeAbsolute) {
                                removeModes.push(QGroundControl.AltitudeModeAbsolute)
                            }
                            removeModes.push(QGroundControl.AltitudeModeMixed)
                            altModeDialogComponent.createObject(mainWindow, { rgRemoveModes: removeModes, updateAltModeFn: updateFunction }).open()
                        }
                    }

                    RowLayout {
                        spacing: _altRectMargin

                        QGCLabel {
                            id:                 altModeLabel
                            Layout.alignment:   Qt.AlignBaseline
                            visible:            _globalAltMode !== QGroundControl.AltitudeModeRelative
                        }
                        QGCColoredImage {
                            height:     ScreenTools.defaultFontPixelHeight / 2
                            width:      height
                            source:     "/res/DropArrow.svg"
                            color:      qgcPal.text
                            visible:    _globalAltModeIsMixed
                        }
                    }
                }

                FactTextFieldSlider {
                    id:                 altField
                    Layout.fillWidth:   true
                    label:              qsTr("Altitude")
                    fact:               missionItem.altitude
                }

                QGCLabel {
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               qsTr("Actual AMSL alt sent: %1 %2").arg(missionItem.amslAltAboveTerrain.valueString).arg(missionItem.amslAltAboveTerrain.units)
                    visible:            missionItem.altitudeMode === QGroundControl.AltitudeModeCalcAboveTerrain
                }
            }

            ColumnLayout {
                width:      parent.width
                spacing:    _margin

                Repeater {
                    model: missionItem.comboboxFacts

                    ColumnLayout {
                        Layout.fillWidth:   true
                        spacing:            0

                        QGCLabel {
                            font.pointSize: ScreenTools.smallFontPointSize
                            text:           object.name
                            visible:        object.name !== ""
                        }

                        FactComboBox {
                            Layout.fillWidth:   true
                            indexModel:         false
                            model:              object.enumStrings
                            fact:               object
                        }
                    }
                }
            }

            Repeater {
                model: missionItem.textFieldFacts

                FactTextFieldSlider {
                    width:          parent.width
                    label:              object.name
                    fact:               object
                    enabled:            !object.readOnly
                }
            }

            Repeater {
                model: missionItem.nanFacts

                FactTextFieldSlider {
                    width:                  parent.width
                    label:                  object.name
                    fact:                   object
                    showEnableCheckbox:     true
                    enableCheckBoxChecked:  !isNaN(object.rawValue)

                    onEnableCheckboxClicked: object.rawValue = enableCheckBoxChecked ? 0 : NaN
                }
            }

            FactTextFieldSlider {
                width:                  parent.width
                label:                  qsTr("Flight Speed")
                fact:                   missionItem.speedSection.flightSpeed
                showEnableCheckbox:     true
                enableCheckBoxChecked:  missionItem.speedSection.specifyFlightSpeed
                visible:                missionItem.speedSection.available

                onEnableCheckboxClicked: missionItem.speedSection.specifyFlightSpeed = enableCheckBoxChecked
            }

            CameraSection {
                width:      parent.width
                visible:    missionItem.cameraSection.available

                Component.onCompleted: checked = missionItem.cameraSection.settingsSpecified
            }
        }
    }
}
