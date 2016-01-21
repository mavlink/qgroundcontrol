import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

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
    property var    qgcView     ///< QGCView control used for showing dialogs

    signal clicked
    signal remove
    signal removeAll
    signal insert(int i)

    height: innerItem.height + (_margin * 3)
    color:  missionItem.isCurrentItem ? qgcPal.buttonHighlight : qgcPal.windowShade
    radius: _radius

    readonly property real _editFieldWidth:     ScreenTools.defaultFontPixelWidth * 16
    readonly property real _margin:             ScreenTools.defaultFontPixelWidth / 2
    readonly property real _radius:             ScreenTools.defaultFontPixelWidth / 2

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    Component {
        id: deleteAllPromptDialog

        QGCViewMessage {
            message: "Are you sure you want to delete all mission items?"

            function accept() {
                removeAll()
                hideDialog()
            }
        }
    }

    Item {
        id:                 innerItem
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             valuesRect.visible ? valuesRect.y + valuesRect.height : valuesRect.y

        MouseArea {
            anchors.fill:   parent
            visible:        !missionItem.isCurrentItem

            onClicked: _root.clicked()
        }

        QGCLabel {
            id:                     label
            anchors.verticalCenter: commandPicker.verticalCenter
            color:                  missionItem.isCurrentItem ? qgcPal.buttonHighlightText : qgcPal.buttonText
            text:                   missionItem.sequenceNumber == 0 ? "H" : missionItem.sequenceNumber
        }

        Image {
            id:                     hamburger
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
            anchors.right:          parent.right
            anchors.verticalCenter: commandPicker.verticalCenter
            width:                  commandPicker.height
            height:                 commandPicker.height
            source:                 "qrc:/qmlimages/Hamburger.svg"
            visible:                missionItem.isCurrentItem && missionItem.sequenceNumber != 0

            MouseArea {
                anchors.fill:   parent
                onClicked:      hamburgerMenu.popup()

                Menu {
                    id: hamburgerMenu

                    MenuItem {
                        text:           "Insert"
                        onTriggered:    insert(missionItem.sequenceNumber)
                    }

                    MenuItem {
                        text:           "Delete"
                        onTriggered:    remove()
                    }

                    MenuItem {
                        text:           "Delete all"

                        onTriggered: qgcView.showDialog(deleteAllPromptDialog, "Delete all", qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                    }

                    MenuSeparator { }

                    MenuItem {
                        text:       "Show all values"
                        checkable:  true
                        checked:    missionItem.rawEdit

                        onTriggered:    {
                            if (missionItem.rawEdit) {
                                if (missionItem.friendlyEditAllowed) {
                                    missionItem.rawEdit = false
                                } else {
                                    qgcView.showMessage("Mission Edit", "You have made changes to the mission item which cannot be shown in Simple Mode", StandardButton.Ok)
                                }
                            } else {
                                missionItem.rawEdit = true
                            }
                            checked = missionItem.rawEdit
                        }
                    }
                }
            }
        }

        QGCButton {
            id:                     commandPicker
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 2
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
            anchors.left:           label.right
            anchors.right:          hamburger.left
            visible:                missionItem.sequenceNumber != 0 && missionItem.isCurrentItem && !missionItem.rawEdit
            text:                   missionItem.commandName

            Component {
                id: commandDialog

                MissionCommandDialog {
                    missionItem: _root.missionItem
                }
            }

            onClicked:              qgcView.showDialog(commandDialog, "Select Mission Command", qgcView.showDialogDefaultWidth, StandardButton.Cancel)
        }

        QGCLabel {
            anchors.fill:       commandPicker
            visible:            missionItem.sequenceNumber == 0 || !missionItem.isCurrentItem
            verticalAlignment:  Text.AlignVCenter
            text:               missionItem.sequenceNumber == 0 ? "Home" : missionItem.commandName
            color:              qgcPal.buttonText
        }

        Rectangle {
            id:                 valuesRect
            anchors.topMargin:  _margin
            anchors.top:        commandPicker.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            height:             valuesItem.height
            color:              qgcPal.windowShadeDark
            visible:            missionItem.sequenceNumber != 0 && missionItem.isCurrentItem
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
                        text:       missionItem.rawEdit ?
                                        "Provides advanced access to all commands/parameters. Be very careful!" :
                                        missionItem.commandDescription
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
                } // Column
            } // Item
        } // Rectangle
    } // Item
} // Rectangle
