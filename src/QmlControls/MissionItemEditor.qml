import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQml
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Mission item edit control
Rectangle {
    id:             _root
    height:         mainLayout.height
    color:          "transparent"
    radius:         _radius
    border.width:   _readyForSave ? 0 : 2
    border.color:   qgcPal.warningText

    property var    map                 ///< Map control
    property var    masterController
    property var    missionItem         ///< MissionItem associated with this editor
    property bool   readOnly: false     ///< true: read only view, false: full editing view

    signal remove
    signal selectNextNotReadyItem

    property var    _masterController:          masterController
    property var    _missionController:         _masterController.missionController
    property color  _outerTextColor:            qgcPal.text
    property bool   _noMissionItemsAdded:       _missionController.visualItems.count === 1
    property real   _sectionSpacer:             ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings
    property bool   _singleComplexItem:         _missionController.complexMissionItemNames.length === 1
    property bool   _readyForSave:              missionItem.readyForSaveState === VisualMissionItem.ReadyForSave

    readonly property real  _editFieldWidth:    Math.min(width - _innerMargin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _innerMargin:       2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _hamburgerSize:     ScreenTools.defaultFontPixelHeight
    readonly property real  _trashSize:         ScreenTools.defaultFontPixelHeight
    readonly property bool  _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    Column {
        id:                 mainLayout
        anchors.margins:    _margin
        anchors.left:       parent.left
        width:              parent.width - (_margin * 2)
        spacing:            ScreenTools.defaultFontPixelHeight / 2

        Loader {
            id:             editorLoader
            source:         missionItem.editorQml
            asynchronous:   true

            property var    masterController:   _masterController
            property real   availableWidth:     parent.width
            property var    editorRoot:         _root
        }

        Rectangle {
            id:         separator
            width:      parent.width
            height:     1
            color:      "transparent"
        }

        RowLayout {
            id:         bottomRowLayout
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelWidth

            Rectangle {
                id:                     notReadyForSaveIndicator
                Layout.preferredWidth:  visible ? _hamburgerSize : 0
                Layout.preferredHeight: _hamburgerSize
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

            QGCColoredImage {
                id:                     deleteButton
                Layout.preferredWidth:  visible ? _hamburgerSize : 0
                Layout.preferredHeight: _hamburgerSize
                Layout.fillHeight:      true
                source:                 "/res/TrashDelete.svg"
                sourceSize.height:      _hamburgerSize
                fillMode:               Image.PreserveAspectFit
                mipmap:                 true
                smooth:                 true
                color:                  qgcPal.text
                visible:                missionItem.sequenceNumber !== 0

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  remove()
                }
            }

            QGCLabel {
                Layout.fillWidth:       true
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                text:                   missionItem.commandDescription
                wrapMode:               Text.WordWrap
                font.pointSize:         ScreenTools.smallFontPointSize
            }

            QGCColoredImage {
                id:                     hamburger
                Layout.alignment:       Qt.AlignRight
                Layout.preferredWidth:  visible ? _hamburgerSize : 0
                Layout.preferredHeight: _hamburgerSize
                sourceSize.height:      _hamburgerSize
                source:                 "qrc:/qmlimages/Hamburger.svg"
                color:                  qgcPal.text
                visible:                missionItem.sequenceNumber !== 0

                QGCMouseArea {
                    fillItem: hamburger

                    onClicked: (position) => {
                        currentItemScope.focus = true
                        position = Qt.point(position.x, position.y)
                        // For some strange reason using mainWindow in mapToItem doesn't work, so we use globals.parent instead which also gets us mainWindow
                        position = mapToItem(globals.parent, position)
                        var dropPanel = hamburgerMenuDropPanelComponent.createObject(mainWindow, { clickRect: Qt.rect(position.x, position.y, 0, 0) })
                        dropPanel.open()
                    }
                }
            }
        }
    }

    Component {
        id: editPositionDialog

        EditPositionDialog {
            coordinate:             missionItem.isSurveyItem ?  missionItem.centerCoordinate : missionItem.coordinate
            onCoordinateChanged:    missionItem.isSurveyItem ?  missionItem.centerCoordinate = coordinate : missionItem.coordinate = coordinate
        }
    }

    Component {
        id: hamburgerMenuDropPanelComponent

        DropPanel {
            id: hamburgerMenuDropPanel

            sourceComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Move to vehicle position")
                        enabled:            _activeVehicle && missionItem.specifiesCoordinate

                        onClicked: {
                            missionItem.coordinate = _activeVehicle.coordinate
                            hamburgerMenuDropPanel.close()
                        }

                        property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Move to previous item position")
                        enabled:            _missionController.previousCoordinate.isValid
                        onClicked: {
                            missionItem.coordinate = _missionController.previousCoordinate
                            hamburgerMenuDropPanel.close()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Edit position...")
                        enabled:            missionItem.specifiesCoordinate
                        onClicked: {
                            editPositionDialog.createObject(mainWindow).open()
                            hamburgerMenuDropPanel.close()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Change command...")
                        onClicked: {
                            commandDialog.createObject(mainWindow).open()
                            hamburgerMenuDropPanel.close()
                        }

                        Component {
                            id: commandDialog

                            MissionCommandDialog {
                                vehicle:                    masterController.controllerVehicle
                                missionItem:                _root.missionItem
                                map:                        _root.map
                                // FIXME: Disabling fly through commands doesn't work since you may need to change from an RTL to something else
                                flyThroughCommandsAllowed:  true //_missionController.flyThroughCommandsAllowed
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth:       true
                        Layout.preferredHeight: 1
                        color:                  qgcPal.groupBorder
                    }

                    QGCCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               qsTr("Show all values")
                        visible:            QGroundControl.corePlugin.showAdvancedUI
                        checked:            missionItem.isSimpleItem ? missionItem.rawEdit : false
                        enabled:            missionItem.isSimpleItem && !_waypointsOnlyMode

                        onClicked: {
                            missionItem.rawEdit = checked
                            if (missionItem.rawEdit && !missionItem.friendlyEditAllowed) {
                                missionItem.rawEdit = false
                                checked = false
                                mainWindow.showMessageDialog(qsTr("Mission Edit"), qsTr("You have made changes to the mission item which cannot be shown in Simple Mode"))
                            }
                            hamburgerMenuDropPanel.close()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth:       true
                        Layout.preferredHeight: 1
                        color:                  qgcPal.groupBorder
                    }

                    QGCLabel {
                        text:       qsTr("Item #%1").arg(missionItem.sequenceNumber)
                        enabled:    false
                    }
                }
            }
        }
    }
} // Rectangle
