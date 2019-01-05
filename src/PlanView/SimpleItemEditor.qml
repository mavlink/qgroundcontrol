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

    property string _altModeRelativeHelpText:       qsTr("Relative to home altitude")
    property string _altModeAbsoluteHelpText:       qsTr("Above Mean Sea Level")
    property string _altModeAboveTerrainHelpText:   qsTr("Calculated from terrain data\nAMSL Alt ") + missionItem.amslAltAboveTerrain.valueString + " " + missionItem.amslAltAboveTerrain.units
    property string _altModeTerrainFrameHelpText:   qsTr("Using terrain reference frame")

    function updateAltitudeModeText() {
        if (missionItem.altitudeMode === _altModeRelative) {
            altModeLabel.text = qsTr("Altitude")
            altModeHelp.text = _altModeRelativeHelpText
        } else if (missionItem.altitudeMode === _altModeAbsolute) {
            altModeLabel.text = qsTr("Above Mean Sea Level")
            altModeHelp.text = _altModeAbsoluteHelpText
        } else if (missionItem.altitudeMode === _altModeAboveTerrain) {
            altModeLabel.text = qsTr("Above Terrain")
            altModeHelp.text = Qt.binding(function() { return _altModeAboveTerrainHelpText })
        } else if (missionItem.altitudeMode === _altModeTerrainFrame) {
            altModeLabel.text = qsTr("Terrain Frame")
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
                        anchors.top:            altModeLabel.top
                        width:                  height
                        height:                 altModeLabel.height
                        sourceSize.height:      height
                        source:                 "qrc:/qmlimages/Hamburger.svg"
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
                            onTriggered:    missionItem.altitudeMode = _altModeRelative
                        }

                        MenuItem {
                            text:           qsTr("Height Above Mean Sea Level")
                            onTriggered:    missionItem.altitudeMode = _altModeAbsolute
                        }

                        MenuItem {
                            text:           qsTr("Height Above Terrain")
                            onTriggered:    missionItem.altitudeMode = _altModeAboveTerrain
                        }

                        MenuItem {
                            text:           qsTr("Terrain Frame")
                            onTriggered:    missionItem.altitudeMode = _altModeTerrainFrame
                        }
                    }
                }

                FactTextField {
                    fact: missionItem.altitude
                }

                QGCLabel {
                    id:             altModeHelp
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    font.pointSize: ScreenTools.smallFontPointSize
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
