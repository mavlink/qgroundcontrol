import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Controls.Styles      1.4
import QtQuick.Dialogs              1.2
import QtQml                        2.2
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0


/// Mission item edit control
Rectangle {
    id:             _root
    height:         editorLoader.visible ? (editorLoader.y + editorLoader.height + (_margin * 2)) : (commandPicker.y + commandPicker.height + _margin / 2)
    color:          _currentItem ? qgcPal.missionItemEditor : qgcPal.windowShade
    radius:         _radius
    opacity:        _currentItem ? 1.0 : 0.7
    border.width:   _readyForSave ? 0 : 1
    border.color:   qgcPal.warningText

    property var    map                 ///< Map control
    property var    masterController
    property var    missionItem         ///< MissionItem associated with this editor
    property bool   readOnly            ///< true: read only view, false: full editing view

    signal clicked
    signal remove
    signal selectNextNotReadyItem

    property var    _masterController:          masterController
    property var    _missionController:         _masterController.missionController
    property bool   _currentItem:               missionItem.isCurrentItem
    property color  _outerTextColor:            _currentItem ? qgcPal.primaryButtonText : qgcPal.text
    property bool   _noMissionItemsAdded:       ListView.view.model.count === 1
    property real   _sectionSpacer:             ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings
    property bool   _singleComplexItem:         _missionController.complexMissionItemNames.length === 1
    property bool   _readyForSave:              missionItem.readyForSaveState === VisualMissionItem.ReadyForSave
    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _hamburgerSize:     commandPicker.height * 0.75
    readonly property real  _trashSize:     commandPicker.height * 0.75
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

    Rectangle {
        id:                     notReadyForSaveIndicator
        anchors.verticalCenter: notReadyForSaveLabel.visible ? notReadyForSaveLabel.verticalCenter : commandPicker.verticalCenter
        anchors.leftMargin:     _margin
        anchors.left:           parent.left
        width:                  readyForSaveLabel.contentHeight
        height:                 width
        border.width:           1
        border.color:           qgcPal.warningText
        color:                  "white"
        radius:                 width / 2
        visible:                !_readyForSave

        QGCLabel {
            id:                 readyForSaveLabel
            anchors.centerIn:   parent
            //: Indicator in Plan view to show mission item is not ready for save/send
            text:               qsTr("?")
            color:              qgcPal.warningText
            font.pointSize:     ScreenTools.smallFontPointSize
        }
    }

    QGCLabel {
        id:                     notReadyForSaveLabel
        anchors.margins:        _margin
        anchors.left:           notReadyForSaveIndicator.right
        anchors.right:          parent.right
        anchors.top:            commandPicker.bottom
        visible:                _currentItem && !_readyForSave
        text:                   missionItem.readyForSaveState === VisualMissionItem.NotReadyForSaveTerrain ?
                                    qsTr("Incomplete: Waiting on terrain data.") :
                                    qsTr("Incomplete: Item not fully specified.")
        wrapMode:               Text.WordWrap
        horizontalAlignment:    Text.AlignHCenter
        color:                  qgcPal.warningText
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

        QGCMenu {
            id: hamburgerMenu

            QGCMenuItem {
                text:           qsTr("Move to vehicle position")
                visible:        missionItem.specifiesCoordinate
                enabled:        _activeVehicle
                onTriggered:    missionItem.coordinate = _activeVehicle.coordinate
            }

            QGCMenuItem {
                text:           qsTr("Move to previous item position")
                visible:        _missionController.previousCoordinate.isValid
                onTriggered:    missionItem.coordinate = _missionController.previousCoordinate
            }

            QGCMenuItem {
                text:           qsTr("Edit position...")
                visible:        missionItem.specifiesCoordinate
                onTriggered:    mainWindow.showComponentDialog(editPositionDialog, qsTr("Edit Position"), mainWindow.showDialogDefaultWidth, StandardButton.Close)
            }

            QGCMenuSeparator {
                visible: missionItem.isSimpleItem && !_waypointsOnlyMode
            }

            QGCMenuItem {
                text:       qsTr("Show all values")
                checkable:  true
                checked:    missionItem.isSimpleItem ? missionItem.rawEdit : false
                visible:    missionItem.isSimpleItem && !_waypointsOnlyMode

                onTriggered:    {
                    if (missionItem.rawEdit) {
                        if (missionItem.friendlyEditAllowed) {
                            missionItem.rawEdit = false
                        } else {
                            mainWindow.showMessageDialog(qsTr("Mission Edit"), qsTr("You have made changes to the mission item which cannot be shown in Simple Mode"))
                        }
                    } else {
                        missionItem.rawEdit = true
                    }
                    checked = missionItem.rawEdit
                }
            }

            QGCMenuItem {
                text:       qsTr("Item #%1").arg(missionItem.sequenceNumber)
                enabled:    false
            }
        }
    }

    QGCColoredImage {
        id:                     deleteButton
        anchors.margins:        _margin
        anchors.left:           parent.left
        anchors.verticalCenter: commandPicker.verticalCenter
        height:                 _hamburgerSize
        width:                  height
        sourceSize.height:      height
        fillMode:               Image.PreserveAspectFit
        mipmap:                 true
        smooth:                 true
        color:                  qgcPal.text
        visible:                _currentItem && missionItem.sequenceNumber !== 0
        source:                 "/res/TrashDelete.svg"

        QGCMouseArea {
            fillItem:   parent
            onClicked:  remove()
        }
    }

    Rectangle {
        id:                 commandPicker
        anchors.margins:    _margin
        anchors.left:       deleteButton.right
        anchors.top:        parent.top
        height:             ScreenTools.implicitComboBoxHeight
        width:              innerLayout.x + innerLayout.width + ScreenTools.comboBoxPadding
        visible:            !commandLabel.visible
        color:              qgcPal.window
        border.width:       1
        border.color:       qgcPal.text

        RowLayout {
            id:                     innerLayout
            anchors.margins:        _padding
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing:                _padding

            property real _padding: ScreenTools.comboBoxPadding

            QGCLabel { text: missionItem.commandName }

            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelWidth
                width:              height
                fillMode:           Image.PreserveAspectFit
                smooth:             true
                antialiasing:       true
                color:              qgcPal.text
                source:             "/qmlimages/arrow-down.png"
            }
        }

        QGCMouseArea {
            fillItem:   parent
            onClicked:  mainWindow.showComponentDialog(commandDialog, qsTr("Select Mission Command"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel)
        }

        Component {
            id: commandDialog

            MissionCommandDialog {
                missionItem:                _root.missionItem
                map:                        _root.map
                // FIXME: Disabling fly through commands doesn't work since you may need to change from an RTL to something else
                flyThroughCommandsAllowed:  true //_missionController.flyThroughCommandsAllowed
            }
        }

    }

    QGCLabel {
        id:                     commandLabel
        anchors.leftMargin:     ScreenTools.comboBoxPadding
        anchors.fill:           commandPicker
        visible:                !missionItem.isCurrentItem || !missionItem.isSimpleItem || _waypointsOnlyMode || missionItem.isTakeoffItem
        verticalAlignment:      Text.AlignVCenter
        text:                   missionItem.commandName
        color:                  _outerTextColor
    }

    Loader {
        id:                 editorLoader
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.top:        _readyForSave ? commandPicker.bottom : notReadyForSaveLabel.bottom
        source:             missionItem.editorQml
        visible:            _currentItem

        property var    masterController:   _masterController
        property real   availableWidth:     _root.width - (_margin * 2) ///< How wide the editor should be
        property var    editorRoot:         _root
    }
} // Rectangle
