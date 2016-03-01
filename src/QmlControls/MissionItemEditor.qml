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

    height: editorLoader.y + editorLoader.height + (_margin * 2)
    color:  _currentItem ? qgcPal.buttonHighlight : qgcPal.windowShade
    radius: _radius

    property var    missionItem ///< MissionItem associated with this editor
    property bool   readOnly    ///< true: read only view, false: full editing view
    property var    qgcView     ///< QGCView control used for showing dialogs

    signal clicked
    signal remove
    signal insert(int i)
    signal moveHomeToMapCenter

    property bool   _currentItem:       missionItem.isCurrentItem
    property color  _outerTextColor:    _currentItem ? "black" : qgcPal.text

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 16)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }


    MouseArea {
        anchors.fill:   parent
        visible:        !missionItem.isCurrentItem
        onClicked:      _root.clicked()
    }

    QGCLabel {
        id:                     label
        anchors.verticalCenter: commandPicker.verticalCenter
        anchors.leftMargin:     _margin
        anchors.left:           parent.left
        text:                   missionItem.sequenceNumber == 0 ? "H" : missionItem.sequenceNumber
        color:                  _outerTextColor
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

                MenuSeparator {
                    visible: missionItem.isSimpleItem
                }

                MenuItem {
                    text:       "Show all values"
                    checkable:  true
                    checked:    missionItem.isSimpleItem ? missionItem.rawEdit : false
                    visible:    missionItem.isSimpleItem

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
        visible:                missionItem.sequenceNumber != 0 && missionItem.isCurrentItem && !missionItem.rawEdit && missionItem.isSimpleItem
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
        visible:            missionItem.sequenceNumber == 0 || !missionItem.isCurrentItem || !missionItem.isSimpleItem
        verticalAlignment:  Text.AlignVCenter
        text:               missionItem.sequenceNumber == 0 ? "Home Position" : (missionItem.isSimpleItem ? missionItem.commandName : "Survey")
        color:              _outerTextColor
    }

    Loader {
        id:                 editorLoader
        anchors.leftMargin: _margin
        anchors.topMargin:  _margin
        anchors.left:       parent.left
        anchors.top:        commandPicker.bottom
        height:             _currentItem && item ? item.height : 0
        source:             _currentItem ? (missionItem.isSimpleItem ? "qrc:/qml/SimpleItemEditor.qml" : "qrc:/qml/SurveyItemEditor.qml") : ""

        property real   availableWidth: _root.width - (_margin * 2) ///< How wide the editor should be
        property var    editorRoot:     _root
    }
} // Rectangle
