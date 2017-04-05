import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0


/// Mission item edit control
Rectangle {
    id:     _root
    height: editorLoader.y + editorLoader.height + (_margin * 2)
    color:  _currentItem ? qgcPal.primaryButton : qgcPal.windowShade
    radius: _radius

    property var    map             ///< Map control
    property var    missionItem     ///< MissionItem associated with this editor
    property bool   readOnly        ///< true: read only view, false: full editing view
    property var    rootQgcView

    signal clicked
    signal remove
    signal insertWaypoint
    signal insertComplexItem(string complexItemName)

    property bool   _currentItem:               missionItem.isCurrentItem
    property color  _outerTextColor:            _currentItem ? qgcPal.primaryButtonText : qgcPal.text
    property bool   _noMissionItemsAdded:       ListView.view.model.count === 1
    property real   _sectionSpacer:             ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _hamburgerSize:     commandPicker.height * 0.75
    readonly property bool  _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      _root.clicked()
    }

    QGCLabel {
        id:                     label
        anchors.verticalCenter: commandPicker.verticalCenter
        anchors.leftMargin:     _margin
        anchors.left:           parent.left
        text:                   missionItem.abbreviation.charAt(0)
        color:                  _outerTextColor
    }

    QGCColoredImage {
        id:                     hamburger
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.right:          parent.right
        anchors.verticalCenter: commandPicker.verticalCenter
        width:                  _hamburgerSize
        height:                 _hamburgerSize
        sourceSize.height:      _hamburgerSize
        source:                 "qrc:/qmlimages/Hamburger.svg"
        visible:                missionItem.isCurrentItem && missionItem.sequenceNumber != 0
        color:                  qgcPal.windowShade

    }

    QGCMouseArea {
        fillItem:   hamburger
        visible:    hamburger.visible
        onClicked:  _waypointsOnlyMode ? waypointsOnlyMenu.popup() : normalMenu.popup()

        Menu {
            id: normalMenu

            MenuItem {
                text:           qsTr("Insert waypoint")
                onTriggered:    insert()
            }

            MenuItem {
                text:           qsTr("Delete")
                onTriggered:    remove()
            }

            Menu {
                id:     normalPatternMenu
                title:  qsTr("Insert pattern")

                Instantiator {
                    model: missionController.complexMissionItemNames

                    onObjectAdded:      normalPatternMenu.insertItem(index, object)
                    onObjectRemoved:    normalPatternMenu.removeItem(object)

                    MenuItem {
                        text:           modelData
                        onTriggered:    insertComplexItem(modelData)
                    }
                }
            }

            MenuItem {
                text:           qsTr("Change command...")
                onTriggered:    commandPicker.clicked()
            }

            MenuSeparator {
                visible: missionItem.isSimpleItem
            }

            MenuItem {
                text:       qsTr("Show all values")
                checkable:  true
                checked:    missionItem.isSimpleItem ? missionItem.rawEdit : false
                visible:    missionItem.isSimpleItem

                onTriggered:    {
                    if (missionItem.rawEdit) {
                        if (missionItem.friendlyEditAllowed) {
                            missionItem.rawEdit = false
                        } else {
                            qgcView.showMessage(qsTr("Mission Edit"), qsTr("You have made changes to the mission item which cannot be shown in Simple Mode"), StandardButton.Ok)
                        }
                    } else {
                        missionItem.rawEdit = true
                    }
                    checked = missionItem.rawEdit
                }
            }
        }

        Menu {
            id: waypointsOnlyMenu

            MenuItem {
                text:           qsTr("Insert waypoint")
                onTriggered:    insertWaypoint()
            }

            MenuItem {
                text:           qsTr("Delete")
                onTriggered:    remove()
            }

            Menu {
                id:     waypointsOnlyPatternMenu
                title:  qsTr("Insert pattern")

                Instantiator {
                    model: missionController.complexMissionItemNames

                    onObjectAdded:      waypointsOnlyPatternMenu.insertItem(index, object)
                    onObjectRemoved:    waypointsOnlyPatternMenu.removeItem(object)

                    MenuItem {
                        text:           modelData
                        onTriggered:    insertComplexItem(modelData)
                    }
                }
            }

        }
    }

    QGCButton {
        id:                     commandPicker
        anchors.topMargin:      _margin / 2
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 2
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.left:           label.right
        anchors.top:            parent.top
        visible:                !commandLabel.visible
        text:                   missionItem.commandName

        Component {
            id: commandDialog

            MissionCommandDialog {
                missionItem: _root.missionItem
            }
        }

        onClicked: qgcView.showDialog(commandDialog, qsTr("Select Mission Command"), qgcView.showDialogDefaultWidth, StandardButton.Cancel)
    }

    QGCLabel {
        id:                 commandLabel
        anchors.fill:       commandPicker
        visible:            !missionItem.isCurrentItem || !missionItem.isSimpleItem || _waypointsOnlyMode
        verticalAlignment:  Text.AlignVCenter
        text:               missionItem.commandName
        color:              _outerTextColor
    }

    Loader {
        id:                 editorLoader
        anchors.leftMargin: _margin
        anchors.topMargin:  _margin
        anchors.left:       parent.left
        anchors.top:        commandPicker.bottom
        height:             item ? item.height : 0
        source:             missionItem.editorQml

        onLoaded: {
            item.visible = Qt.binding(function() { return _currentItem; })
        }

        property real   availableWidth: _root.width - (_margin * 2) ///< How wide the editor should be
        property var    editorRoot:     _root
    }
} // Rectangle
