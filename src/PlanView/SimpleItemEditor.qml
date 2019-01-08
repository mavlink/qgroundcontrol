import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Simple mission items
Rectangle {
    width:  availableWidth
    height: valuesColumn.height + (_margin * 2)
    color:  qgcPal.windowShadeDark
    radius: _radius

    readonly property int _altModeRelative:     0
    readonly property int _altModeAbsolute:     1
    readonly property int _altModeAboveTerrain: 2
    readonly property int _altModeTerrainFrame: 3

    property bool _specifiesAltitude:       missionItem.specifiesAltitude
    property real _margin:                  ScreenTools.defaultFontPixelHeight / 2
    property bool _supportsTerrainFrame:    missionItem

    property string _altModeRelativeHelpText:       qsTr("Altitude relative to home altitude")
    property string _altModeAbsoluteHelpText:       qsTr("Altitude above mean sea level")
    property string _altModeAboveTerrainHelpText:   qsTr("Altitude above terrain\nActual AMSL altitude: %1 %2").arg(missionItem.amslAltAboveTerrain.valueString).arg(missionItem.amslAltAboveTerrain.units)
    property string _altModeTerrainFrameHelpText:   qsTr("Using terrain reference frame")

    readonly property string _altModeRelativeExtraUnits:        qsTr(" (Rel)")
    readonly property string _altModeAbsoluteExtraUnits:        qsTr(" (AMSL)")
    readonly property string _altModeAboveTerrainExtraUnits:    qsTr(" (Abv Terr)")
    readonly property string _altModeTerrainFrameExtraUnits:    qsTr(" (TerrF)")

    function updateAltitudeModeText() {
        if (missionItem.altitudeMode === _altModeRelative) {
            altModeLabel.text = qsTr("Altitude")
            altModeHelp.text = _altModeRelativeHelpText
            altField.extraUnits = _altModeRelativeExtraUnits
        } else if (missionItem.altitudeMode === _altModeAbsolute) {
            altModeLabel.text = qsTr("Above Mean Sea Level")
            altModeHelp.text = _altModeAbsoluteHelpText
            altField.extraUnits = _altModeAbsoluteExtraUnits
        } else if (missionItem.altitudeMode === _altModeAboveTerrain) {
            altModeLabel.text = qsTr("Above Terrain")
            altModeHelp.text = Qt.binding(function() { return _altModeAboveTerrainHelpText })
            altField.extraUnits = _altModeAboveTerrainExtraUnits
        } else if (missionItem.altitudeMode === _altModeTerrainFrame) {
            altModeLabel.text = qsTr("Terrain Frame")
            altModeHelp.text = _altModeTerrainFrameHelpText
            altField.extraUnits = _altModeTerrainFrameExtraUnits
        } else {
            altModeLabel.text = qsTr("Internal Error")
            altModeHelp.text = ""
            altField.extraUnits = ""
        }
    }

    Component.onCompleted: updateAltitudeModeText()

    Connections {
        target:                 missionItem
        onAltitudeModeChanged:  updateAltitudeModeText()
    }

    Column {
        id:                 valuesColumn
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

        GridLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            columns:        2

            Repeater {
                model: missionItem.comboboxFacts

                QGCLabel {
                    text:           object.name
                    visible:        object.name !== ""
                    Layout.column:  0
                    Layout.row:     index
                }
            }

            Repeater {
                model: missionItem.comboboxFacts

                FactComboBox {
                    indexModel:         false
                    model:              object.enumStrings
                    fact:               object
                    Layout.column:      1
                    Layout.row:         index
                    Layout.fillWidth:   true
                }
            }
        }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         altColumn.y + altColumn.height + _margin
            color:          qgcPal.windowShade
            visible:        _specifiesAltitude

            Column {
                id:                 altColumn
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin

                Item {
                    width:  altHamburger.x + altHamburger.width
                    height: altModeLabel.height

                    QGCLabel { id: altModeLabel }

                    QGCColoredImage {
                        id:                     altHamburger
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
                        onClicked:      altHamburgerMenu.popup()
                    }

                    Menu {
                        id: altHamburgerMenu

                        MenuItem {
                            text:           qsTr("Altitude Relative To Home")
                            checkable:      true
                            checked:        missionItem.altitudeMode === _altModeRelative
                            onTriggered:    missionItem.altitudeMode = _altModeRelative
                        }

                        MenuItem {
                            text:           qsTr("Altitude Above Mean Sea Level")
                            checkable:      true
                            checked:        missionItem.altitudeMode === _altModeAbsolute
                            visible:        QGroundControl.corePlugin.options.showMissionAbsoluteAltitude
                            onTriggered:    missionItem.altitudeMode = _altModeAbsolute
                        }

                        MenuItem {
                            text:           qsTr("Altitude Above Terrain")
                            checkable:      true
                            checked:        missionItem.altitudeMode === _altModeAboveTerrain
                            onTriggered:    missionItem.altitudeMode = _altModeAboveTerrain
                            visible:        missionItem.specifiesCoordinate
                        }

                        MenuItem {
                            text:           qsTr("Terrain Frame")
                            checkable:      true
                            checked:        missionItem.altitudeMode === _altModeTerrainFrame
                            visible:        missionItem.altitudeMode === _altModeTerrainFrame
                            onTriggered:    missionItem.altitudeMode = _altModeTerrainFrame
                        }
                    }
                }

                FactTextField {
                    id:                 altField
                    fact:               missionItem.altitude
                    unitsLabel:         fact.units + extraUnits
                    anchors.left:       parent.left
                    anchors.right:      parent.right

                    property string extraUnits
                }

                QGCLabel {
                    id:                 altModeHelp
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.smallFontPointSize
                    anchors.left:       parent.left
                    anchors.right:      parent.right
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
    } // Column
} // Rectangle
