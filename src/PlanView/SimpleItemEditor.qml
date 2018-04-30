import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

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

    property bool _specifiesAltitude:   missionItem.specifiesAltitude
    property bool _altModeIsTerrain:    missionItem.altitudeMode === 2
    property real _margin:              ScreenTools.defaultFontPixelHeight / 2

    ExclusiveGroup {
        id: altRadios
        onCurrentChanged: missionItem.altitudeMode = current.value
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
            anchors.left:           parent.left
            anchors.right:          parent.right
            height:                 altColumn.y + altColumn.height + _margin
            color:                  qgcPal.windowShade

            Column {
                id:                 altColumn
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin

                QGCLabel {
                    font.pointSize: ScreenTools.smallFontPointSize
                    text:           qsTr("Altitude")
                }

                RowLayout {
                    QGCRadioButton { text: qsTr("Rel"); exclusiveGroup: altRadios; checked: missionItem.altitudeMode === value; readonly property int value: 0 }
                    QGCRadioButton { text: qsTr("Abs"); exclusiveGroup: altRadios; checked: missionItem.altitudeMode === value; readonly property int value: 1 }
                    QGCRadioButton { text: qsTr("AGL"); exclusiveGroup: altRadios; checked: missionItem.altitudeMode === value; readonly property int value: 2 }
                }

                FactValueSlider {
                    fact:           missionItem.altitude
                    digitCount:     3
                    incrementSlots: 1
                    visible:        _specifiesAltitude
                }

                RowLayout {
                    spacing: _margin

                    QGCLabel {
                        text:           qsTr("Calculated Abs Alt")
                        font.pointSize: ScreenTools.smallFontPointSize
                        visible:        _altModeIsTerrain
                    }
                    QGCLabel {
                        text:       missionItem.amslAltAboveTerrain.valueString + " " + missionItem.amslAltAboveTerrain.units
                        visible:    _altModeIsTerrain
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
    } // Column
} // Rectangle
