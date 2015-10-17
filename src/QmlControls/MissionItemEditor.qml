import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0


/// Mission item edit control
Rectangle {
    id: _root

    property var    missionItem ///< MissionItem associated with this editor
    property bool   readOnly    ///< true: read only view, false: full editing view

    signal clicked
    signal remove

    height: innerItem.height + (_margin * 2)
    color:  missionItem.isCurrentItem ? qgcPal.buttonHighlight : qgcPal.windowShade
    radius: _radius

    readonly property real _editFieldWidth:     ScreenTools.defaultFontPixelWidth * 16
    readonly property real _margin:             ScreenTools.defaultFontPixelWidth / 2
    readonly property real _radius:             ScreenTools.defaultFontPixelWidth / 2

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    Item {
        id:                 innerItem
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             valuesRect.visible ? valuesRect.y + valuesRect.height : valuesRect.y

        MissionItemIndexLabel {
            id:                     label
            anchors.verticalCenter: commandPicker.verticalCenter
            isCurrentItem:          missionItem.isCurrentItem
            label:                  missionItem.sequenceNumber == 0 ? "H" : missionItem.sequenceNumber
        }

        MouseArea {
            anchors.fill:   parent
            visible:        !missionItem.isCurrentItem

            onClicked: _root.clicked()
        }

        QGCComboBox {
            id:                 commandPicker
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 10
            anchors.left:       label.right
            anchors.right:      parent.right
            currentIndex:       missionItem.commandByIndex
            model:              missionItem.commandNames
            visible:            missionItem.sequenceNumber != 0 && missionItem.isCurrentItem

            onActivated: missionItem.commandByIndex = index
        }

        Rectangle {
            anchors.fill:   commandPicker
            color:          qgcPal.button
            visible:        !commandPicker.visible

            QGCLabel {
                id:                 homeLabel
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth
                anchors.fill:       parent
                verticalAlignment:  Text.AlignVCenter
                text:               missionItem.sequenceNumber == 0 ? "Home" : missionItem.commandName
                color:              qgcPal.buttonText
            }
        }

        Rectangle {
            id:                 valuesRect
            anchors.topMargin:  _margin
            anchors.top:        commandPicker.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            height:             valuesItem.height
            color:              qgcPal.windowShadeDark
            visible:            missionItem.isCurrentItem
            radius:             _radius

            Item {
                id:                 valuesItem
                anchors.margins:    _margin
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                height:             valuesColumn.height + _margin

                Column {
                    id:             valuesColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    spacing:        _margin

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       missionItem.commandDescription
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
                            id:     textField
                            text:   object.name
                            fact:   object
                        }
                    }
                } // Column
            } // Item
        } // Rectangle
    } // Item
} // Rectangle
