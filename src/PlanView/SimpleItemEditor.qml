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
    height: valuesColumn.height + _margin
    color:  qgcPal.windowShadeDark
    radius: _radius

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
                    visible:        object.name != ""
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

        GridLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            flow:           GridLayout.TopToBottom
            rows:           missionItem.textFieldFacts.count + missionItem.nanFacts.count + (missionItem.speedSection.available ? 1 : 0)
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

        Repeater {
            model: missionItem.checkboxFacts

            FactCheckBox {
                text:   object.name
                fact:   object
            }
        }

        CameraSection {
            checked:    missionItem.cameraSection.settingsSpecified
            visible:    missionItem.cameraSection.available
        }
    } // Column
} // Rectangle
