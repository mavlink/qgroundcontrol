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
    height: editorLoader.visible ? (editorLoader.y + editorLoader.height + (_margin * 2)) : (commandPicker.y + commandPicker.height + _margin / 2)
    color:  _currentItem ? qgcPal.missionItemEditor : qgcPal.windowShade
    radius: _radius

    property var    map                 ///< Map control
    property var    masterController
    property var    missionItem         ///< MissionItem associated with this editor
    property bool   readOnly            ///< true: read only view, false: full editing view
    property var    rootQgcView

    signal clicked
    signal remove
    signal insertWaypoint
    signal insertComplexItem(string complexItemName)

    property var    _masterController:          masterController
    property var    _missionController:         _masterController.missionController
    property bool   _currentItem:               missionItem.isCurrentItem
    property color  _outerTextColor:            _currentItem ? qgcPal.primaryButtonText : qgcPal.text
    property bool   _noMissionItemsAdded:       ListView.view.model.count === 1
    property real   _sectionSpacer:             ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings
    property bool   _singleComplexItem:         _missionController.complexMissionItemNames.length === 1

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _hamburgerSize:     commandPicker.height * 0.75
    readonly property bool  _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    FocusScope {
        id:             currentItemScope
        anchors.fill:   parent

        MouseArea {
            anchors.fill:   parent
            onClicked: {
                currentItemScope.focus = true
                _root.clicked()
            }
        }
    }

    Component {
        id: editPositionDialog

        EditPositionDialog {
            coordinate: missionItem.coordinate
            onCoordinateChanged: missionItem.coordinate = coordinate
        }
    }

    QGCLabel {
        id:                     label
        anchors.verticalCenter: commandPicker.verticalCenter
        anchors.leftMargin:     _margin
        anchors.left:           parent.left
        text:                   missionItem.homePosition ? "P" : missionItem.sequenceNumber
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
        visible:                missionItem.isCurrentItem && missionItem.sequenceNumber !== 0
        color:                  qgcPal.text
    }

    QGCMouseArea {
        fillItem:   hamburger
        visible:    hamburger.visible
        onClicked: {
            currentItemScope.focus = true
            hamburgerMenu.popup()
        }

        Menu {
            id: hamburgerMenu

            MenuItem {
                text:           qsTr("Insert waypoint")
                onTriggered:    insertWaypoint()
            }

            Menu {
                id:         patternMenu
                title:      qsTr("Insert pattern")
                visible:    !_singleComplexItem

                Instantiator {
                    model: _missionController.complexMissionItemNames

                    onObjectAdded:      patternMenu.insertItem(index, object)
                    onObjectRemoved:    patternMenu.removeItem(object)

                    MenuItem {
                        text:           modelData
                        onTriggered:    insertComplexItem(modelData)
                    }
                }
            }

            MenuItem {
                text:           qsTr("Insert ") + _missionController.complexMissionItemNames[0]
                visible:        _singleComplexItem
                onTriggered:    insertComplexItem(_missionController.complexMissionItemNames[0])
            }

            MenuItem {
                text:           qsTr("Delete")
                onTriggered:    remove()
            }

            MenuItem {
                text:           qsTr("Change command...")
                onTriggered:    commandPicker.clicked()
                visible:        missionItem.isSimpleItem && !_waypointsOnlyMode
            }

            MenuItem {
                text:           qsTr("Edit position...")
                visible:        missionItem.specifiesCoordinate
                onTriggered:    qgcView.showDialog(editPositionDialog, qsTr("Edit Position"), qgcView.showDialogDefaultWidth, StandardButton.Close)
            }

            MenuSeparator {
                visible: missionItem.isSimpleItem && !_waypointsOnlyMode
            }

            MenuItem {
                text:       qsTr("Show all values")
                checkable:  true
                checked:    missionItem.isSimpleItem ? missionItem.rawEdit : false
                visible:    missionItem.isSimpleItem && !_waypointsOnlyMode

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
        source:             missionItem.editorQml
        visible:            _currentItem

        property var    masterController:   _masterController
        property real   availableWidth:     _root.width - (_margin * 2) ///< How wide the editor should be
        property var    editorRoot:         _root
    }
} // Rectangle
