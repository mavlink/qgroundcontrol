import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Simple mission items
Rectangle {
    id:                 valuesRect
    width:              availableWidth
    height:             deferedload.status == Loader.Ready ? (visible ? deferedload.item.height : 0) : 0
    color:              qgcPal.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    Loader {
        id:              deferedload
        active:          valuesRect.visible
        asynchronous:    true
        anchors.margins: _margin
        anchors.left:    valuesRect.left
        anchors.right:   valuesRect.right
        anchors.top:     valuesRect.top
        sourceComponent: Component {
            Item {
                id:                 valuesItem
                height:             valuesColumn.height + (_margin * 2)

                Column {
                    id:             valuesColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    spacing:        _margin

                    QGCLabel {
                        width:          parent.width
                        wrapMode:       Text.WordWrap
                        font.pointSize: ScreenTools.smallFontPointSize
                        text:           missionItem.sequenceNumber == 0 ?
                                            qsTr("Planned home position. Actual home position set by Vehicle.") :
                                            (missionItem.rawEdit ?
                                                qsTr("Provides advanced access to all commands/parameters. Be very careful!") :
                                                missionItem.commandDescription)
                    }

                    Repeater {
                        model: missionItem.comboboxFacts

                        Item {
                            width:  valuesColumn.width
                            height: comboBoxFact.height

                            QGCLabel {
                                id:                 comboBoxLabel
                                anchors.baseline:   comboBoxFact.baseline
                                text:               object.name
                                visible:            object.name != ""
                            }

                            FactComboBox {
                                id:             comboBoxFact
                                anchors.right:  parent.right
                                width:          comboBoxLabel.visible ? _editFieldWidth : parent.width
                                indexModel:     false
                                model:          object.enumStrings
                                fact:           object
                            }
                        }
                    }

                    Repeater {
                        model: missionItem.textFieldFacts

                        Item {
                            width:  valuesColumn.width
                            height: textField.height

                            QGCLabel {
                                id:                 textFieldLabel
                                anchors.baseline:   textField.baseline
                                text:               object.name
                            }

                            FactTextField {
                                id:             textField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                showUnits:      true
                                fact:           object
                                visible:        !_root.readOnly
                            }

                            FactLabel {
                                anchors.baseline:   textFieldLabel.baseline
                                anchors.right:      parent.right
                                fact:               object
                                visible:            _root.readOnly
                            }
                        }
                    }

                    Repeater {
                        model: missionItem.checkboxFacts

                        FactCheckBox {
                            text:   object.name
                            fact:   object
                        }
                    }

                    QGCButton {
                        text:       qsTr("Move Home to map center")
                        visible:    missionItem.homePosition
                        onClicked:  editorRoot.moveHomeToMapCenter()
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                } // Column
            } // Item
        } // Component
    } // Loader
} // Rectangle
