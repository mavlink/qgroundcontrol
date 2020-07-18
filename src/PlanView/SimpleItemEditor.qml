import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Controls.Styles      1.4
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Simple mission items
Rectangle {
    width:  availableWidth
    height: editorColumn.height + (_margin * 2)
    color:  qgcPal.windowShadeDark
    radius: _radius

    property bool _specifiesAltitude:       missionItem.specifiesAltitude
    property real _margin:                  ScreenTools.defaultFontPixelHeight / 2
    property var  _controllerVehicle:       missionItem.masterController.controllerVehicle
    property bool _supportsTerrainFrame:    _controllerVehicle.supportsTerrainFrame
    property int  _globalAltMode:           missionItem.masterController.missionController.globalAltitudeMode
    property bool _globalAltModeIsMixed:    _globalAltMode == QGroundControl.AltitudeModeNone
    property real _radius:                  ScreenTools.defaultFontPixelWidth / 2

    property string _altModeRelativeHelpText:       qsTr("Altitude relative to launch altitude")
    property string _altModeAbsoluteHelpText:       qsTr("Altitude above mean sea level")
    property string _altModeAboveTerrainHelpText:   qsTr("Altitude above terrain\nActual AMSL altitude: %1 %2").arg(missionItem.amslAltAboveTerrain.valueString).arg(missionItem.amslAltAboveTerrain.units)
    property string _altModeTerrainFrameHelpText:   qsTr("Using terrain reference frame")

    function updateAltitudeModeText() {
        if (missionItem.altitudeMode === QGroundControl.AltitudeModeRelative) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeRelative)
            altModeHelp.text = _altModeRelativeHelpText
        } else if (missionItem.altitudeMode === QGroundControl.AltitudeModeAbsolute) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAbsolute)
            altModeHelp.text = _altModeAbsoluteHelpText
        } else if (missionItem.altitudeMode === QGroundControl.AltitudeModeAboveTerrain) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAboveTerrain)
            altModeHelp.text = Qt.binding(function() { return _altModeAboveTerrainHelpText })
        } else if (missionItem.altitudeMode === QGroundControl.AltitudeModeTerrainFrame) {
            altModeLabel.text = QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeTerrainFrame)
            altModeHelp.text = _altModeTerrainFrameHelpText
        } else {
            altModeLabel.text = qsTr("Internal Error")
            altModeHelp.text = ""
        }
    }

    Component.onCompleted: updateAltitudeModeText()

    Connections {
        target:                 missionItem
        onAltitudeModeChanged:  updateAltitudeModeText()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        spacing:            _margin

        QGCLabel {
            width:          parent.width
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text:           missionItem.rawEdit ?
                                qsTr("Provides advanced access to all commands/parameters. Be very careful!") :
                                missionItem.commandDescription
        }

        ColumnLayout {
            anchors.left:       parent.left
            anchors.right:      parent.right
            spacing:            _margin
            visible:            missionItem.isTakeoffItem && missionItem.wizardMode // Hack special case for takeoff item

            QGCLabel {
                text:               qsTr("Move '%1' Takeoff to the %2 location.").arg(_controllerVehicle.vtol ? qsTr("V") : qsTr("T")).arg(_controllerVehicle.vtol ? qsTr("desired") : qsTr("climbout"))
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
                    // Trial of no auto select next item
                    //editorRoot.selectNextNotReadyItem()
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
            anchors.left:       parent.left
            anchors.right:      parent.right
            spacing:            _margin
            visible:            !missionItem.wizardMode

            // This control needs to morph between a simple altitude entry field to a more complex alt mode picker based on the global plan alt mode
            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         altColumn.y + altColumn.height + _margin
                color:          _globalAltModeIsMixed ? qgcPal.windowShade: qgcPal.window
                visible:        _specifiesAltitude

                ColumnLayout {
                    id:                 altColumn
                    anchors.margins:    _globalAltModeIsMixed ? _margin : 0
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    spacing:            _globalAltModeIsMixed ? _margin : 0

                    QGCLabel {
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        font.pointSize:     ScreenTools.smallFontPointSize
                        text:               qsTr("Altitude below specifies the approximate altitude of the ground. Normally 0 for landing back at original launch location.")
                        visible:            missionItem.isLandCommand
                    }

                    Item {
                        width:      altModeDropArrow.x + altModeDropArrow.width
                        height:     altModeLabel.height
                        visible:    _globalAltModeIsMixed

                        QGCLabel { id: altModeLabel }

                        QGCColoredImage {
                            id:                     altModeDropArrow
                            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 4
                            anchors.left:           altModeLabel.right
                            anchors.verticalCenter: altModeLabel.verticalCenter
                            width:                  ScreenTools.defaultFontPixelHeight / 2
                            height:                 width
                            sourceSize.height:      height
                            source:                 "/res/DropArrow.svg"
                            color:                  qgcPal.text
                        }

                        QGCMouseArea {
                            anchors.fill:   parent
                            onClicked:      altModeMenu.popup()
                        }

                        QGCMenu {
                            id: altModeMenu

                            QGCMenuItem {
                                text:           QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeRelative)
                                checkable:      true
                                checked:        missionItem.altitudeMode === QGroundControl.AltitudeModeRelative
                                onTriggered:    missionItem.altitudeMode = QGroundControl.AltitudeModeRelative
                            }

                            QGCMenuItem {
                                text:           QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAbsolute)
                                checkable:      true
                                checked:        missionItem.altitudeMode === QGroundControl.AltitudeModeAbsolute
                                visible:        QGroundControl.corePlugin.options.showMissionAbsoluteAltitude
                                onTriggered:    missionItem.altitudeMode = QGroundControl.AltitudeModeAbsolute
                            }

                            QGCMenuItem {
                                text:           QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAboveTerrain)
                                checkable:      true
                                checked:        missionItem.altitudeMode === QGroundControl.AltitudeModeAboveTerrain
                                onTriggered:    missionItem.altitudeMode = QGroundControl.AltitudeModeAboveTerrain
                                visible:        missionItem.specifiesCoordinate
                            }

                            QGCMenuItem {
                                text:           QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeTerrainFrame)
                                checkable:      true
                                checked:        missionItem.altitudeMode === QGroundControl.AltitudeModeTerrainFrame
                                visible:        _supportsTerrainFrame && (missionItem.specifiesCoordinate || missionItem.specifiesAltitudeOnly)
                                onTriggered:    missionItem.altitudeMode = QGroundControl.AltitudeModeTerrainFrame
                            }
                        }
                    }

                    QGCLabel {
                        text:           qsTr("Altitude")
                        font.pointSize: ScreenTools.smallFontPointSize
                        visible:        !_globalAltModeIsMixed
                    }

                    AltitudeFactTextField {
                        id:                 altField
                        Layout.fillWidth:   true
                        fact:               missionItem.altitude
                        altitudeMode:       missionItem.altitudeMode
                    }

                    QGCLabel {
                        id:                 altModeHelp
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        font.pointSize:     ScreenTools.smallFontPointSize
                        visible:            _globalAltModeIsMixed
                    }
                }
            }

            ColumnLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

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

            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                flow:           GridLayout.TopToBottom
                rows:           missionItem.textFieldFacts.count +
                                missionItem.nanFacts.count +
                                (missionItem.speedSection.available ? 1 : 0)
                columns:        2

                Repeater {
                    model: missionItem.textFieldFacts

                    QGCLabel { text: object.name }
                }

                Repeater {
                    model: missionItem.nanFacts

                    QGCCheckBox {
                        text:           object.name
                        checked:        !isNaN(object.rawValue)
                        onClicked:      object.rawValue = checked ? 0 : NaN
                    }
                }

                QGCCheckBox {
                    id:         flightSpeedCheckbox
                    text:       qsTr("Flight Speed")
                    checked:    missionItem.speedSection.specifyFlightSpeed
                    onClicked:  missionItem.speedSection.specifyFlightSpeed = checked
                    visible:    missionItem.speedSection.available
                }


                Repeater {
                    model: missionItem.textFieldFacts

                    FactTextField {
                        showUnits:          true
                        fact:               object
                        Layout.fillWidth:   true
                        enabled:            !object.readOnly
                    }
                }

                Repeater {
                    model: missionItem.nanFacts

                    FactTextField {
                        showUnits:          true
                        fact:               object
                        Layout.fillWidth:   true
                        enabled:            !isNaN(object.rawValue)
                    }
                }

                FactTextField {
                    fact:               missionItem.speedSection.flightSpeed
                    Layout.fillWidth:   true
                    enabled:            flightSpeedCheckbox.checked
                    visible:            missionItem.speedSection.available
                }
            }

            CameraSection {
                checked:    missionItem.cameraSection.settingsSpecified
                visible:    missionItem.cameraSection.available
            }
        }
    }
}
