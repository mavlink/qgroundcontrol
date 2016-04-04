import QtQuick                  2.2
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
    height:             valuesItem.height
    color:              qgcPal.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    // The following properties must be available up the hierachy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    Item {
        id:                 valuesItem
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
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
                font.pixelSize: ScreenTools.smallFontPixelHeight
                text:           missionItem.sequenceNumber == 0 ?
                                    qsTr("Planned home position.") :
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
} // Rectangle
