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

    property var    missionItem

    signal clicked
    signal remove
    signal moveUp
    signal moveDown

// FIXME: THis doesn't work right for RTL
    height: missionItem.isCurrentItem ?
                ((missionItem.factCount + (missionItem.specifiesCoordinate ? 3 : 0)) * (latitudeField.height + _margin)) + commandPicker.height + deleteButton.height + (_margin * 6) :
                commandPicker.height + (_margin * 2)
    color:  missionItem.isCurrentItem ? qgcPal.buttonHighlight : qgcPal.windowShade

    readonly property real _editFieldWidth:     ScreenTools.defaultFontPixelWidth * 13
    readonly property real _margin:             ScreenTools.defaultFontPixelWidth / 3

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    Item {
        anchors.margins:    _margin
        anchors.fill:       parent

        MissionItemIndexLabel {
            id:                     label
            anchors.verticalCenter: commandPicker.verticalCenter
            isCurrentItem:          missionItem.isCurrentItem
            label:                  missionItem.sequenceNumber
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

            onActivated: missionItem.commandByIndex = index
        }

        Rectangle {
            anchors.margins:    _margin
            anchors.top:        commandPicker.bottom
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            color:              qgcPal.windowShadeDark
            visible:            missionItem.isCurrentItem

            Item {
                anchors.margins:    _margin
                anchors.fill:   parent

                QGCTextField {
                    id:             latitudeField
                    anchors.right:  parent.right
                    width:          _editFieldWidth
                    text:           missionItem.coordinate.latitude
                    visible:        missionItem.specifiesCoordinate

                    onAccepted:     missionItem.coordinate.latitude = text
                }

                QGCTextField {
                    id:                 longitudeField
                    anchors.topMargin:  _margin
                    anchors.top:        latitudeField.bottom
                    anchors.right:      parent.right
                    width:              _editFieldWidth
                    text:               missionItem.coordinate.longitude
                    visible:            missionItem.specifiesCoordinate

                    onAccepted:         missionItem.coordinate.longtitude = text
                }

                QGCTextField {
                    id:                 altitudeField
                    anchors.topMargin:  _margin
                    anchors.top:        longitudeField.bottom
                    anchors.right:      parent.right
                    width:              _editFieldWidth
                    text:               missionItem.coordinate.altitude
                    visible:            missionItem.specifiesCoordinate
                    showUnits:          true
                    unitsLabel:         "meters"

                    onAccepted:     missionItem.coordinate.altitude = text
                }

                QGCLabel {
                    anchors.left:       parent.left
                    anchors.baseline:   latitudeField.baseline
                    text:               "Lat:"
                    visible:            missionItem.specifiesCoordinate
                }

                QGCLabel {
                    anchors.left:       parent.left
                    anchors.baseline:   longitudeField.baseline
                    text:               "Long:"
                    visible:            missionItem.specifiesCoordinate
                }

                QGCLabel {
                    anchors.left:       parent.left
                    anchors.baseline:   altitudeField.baseline
                    text:               "Alt:"
                    visible:            missionItem.specifiesCoordinate
                }

                Column {
                    id:                 valueColumn
                    anchors.topMargin:  _margin
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        missionItem.specifiesCoordinate ? altitudeField.bottom : parent.top
                    spacing:            _margin

                    Repeater {
                        model: missionItem.facts

                        Item {
                            width:  valueColumn.width
                            height: textField.height

                            QGCLabel {
                                anchors.baseline:   textField.baseline
                                text:               object.name
                            }

                            FactTextField {
                                id:             textField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                showUnits:      true
                                fact:           object
                            }
                        }
                    }
                } // Column - Values column

                Row {
                    anchors.topMargin:  _margin
                    anchors.top:        valueColumn.bottom

                    width:      parent.width
                    spacing:    _margin

                    readonly property real buttonWidth: (width - (_margin * 2)) / 3

                    QGCButton {
                        id:     deleteButton
                        width:  parent.buttonWidth
                        text:   "Delete"

                        onClicked: _root.remove()
                    }

                    QGCButton {
                        width:  parent.buttonWidth
                        text:   "Up"

                        onClicked: _root.moveUp()
                    }

                    QGCButton {
                        width:  parent.buttonWidth
                        text:   "Down"

                        onClicked: _root.moveDown()
                    }
                }
            } // Item
        } // Rectangle
    } // Item
} // Rectangle
