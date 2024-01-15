import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.ScreenTools
import QGroundControl.FactControls

Item {
    id:     rootItem
    width:  toolStripPanelVideo.width
    height: toolStripPanelVideo.height

    property alias maxWidth:                 toolStripPanelVideo.maxWidth
    property real  _margins:                 ScreenTools.defaultFontPixelWidth * 0.75
    property bool  _modesPanelVisible:       modesToolStripAction.checked
    property bool  _actionsPanelVisible:     actionsToolStripAction.checked
    property bool  _selectPanelVisible:      selectToolStripAction.checked
    property bool  _actionsMapPanelVisible:  mapToolsToolStripAction.checked && mapToolsToolStripAction.enabled
    property var   _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property var   _gimbalController:        _activeVehicle ? _activeVehicle.gimbalController : undefined

    Connections {
        // Setting target to null makes this connection efectively disabled, dealing with qml warnings
        target: _gimbalController ? _gimbalController : null
        onShowAcquireGimbalControlPopup: {
            showAcquireGimbalControlPopup()
        }
    }

    function showAcquireGimbalControlPopup() {
        // TODO: we should mention who is currently in control
        mainWindow.showMessageDialog(
            title,
            qsTr("Do you want to take over gimbal control?"),
            StandardButton.Yes | StandardButton.Cancel,
            function() {
               _activeVehicle.gimbalController.acquireGimbalControl()
            }
        )
    }

    ToolStripHorizontal {
        id:                toolStripPanelVideo
        model:             toolStripActionList.model
        forceImageScale11: true

        property bool panelHidden: false

        function togglePanelVisibility() {
            if (panelHidden) {
                panelHidden = false
            } else {
                panelHidden = true
            }
        }

        ToolStripActionList {
            id: toolStripActionList
            model: [
                ToolStripAction {
                    text:               toolStripPanelVideo.panelHidden ? qsTr("Show") : qsTr("Hide")
                    iconSource:         "/HA_Icons/PAYLOAD.png"
                    onTriggered:        toolStripPanelVideo.togglePanelVisibility()
                },
                ToolStripAction {
                    id:                 modesToolStripAction
                    text:               qsTr("")
                    iconSource:         "/HA_Icons/MODES.png"
                    checkable:          true
                    visible:            !toolStripPanelVideo.panelHidden

                    onVisibleChanged: {
                        checked = false
                    }
                },
                ToolStripAction {
                    id:                actionsToolStripAction
                    text:              qsTr("")
                    iconSource:        "/HA_Icons/ACTIONS.png"
                    checkable:         true
                    visible:           !toolStripPanelVideo.panelHidden

                    onVisibleChanged: {
                        checked = false
                    }
                },
                // Change here based on control status?
                ToolStripAction {
                    text:              hasControl ? qsTr("Release C.") : qsTr("Acquuire C.")
                    iconSource:        "/HA_Icons/PAYLOAD.png"
                    checkable:         false
                    visible:           !toolStripPanelVideo.panelHidden && _activeVehicle && _gimbalController.activeGimbal
                    onTriggered:       _activeVehicle ?
                                            hasControl ? _gimbalController.releaseGimbalControl() : _gimbalController.acquireGimbalControl()
                                                : undefined

                    property var hasControl: _gimbalController && _gimbalController.activeGimbal && _gimbalController.activeGimbal.gimbalHaveControl
                },
                ToolStripAction {
                    id:                 selectToolStripAction
                    text:               qsTr("Gimbal ") + (_gimbalController && _gimbalController.activeGimbal ? _gimbalController.activeGimbal.deviceId : "")
                    iconSource:         "/HA_Icons/SELECT.png"
                    checkable:          true
                    visible:            !toolStripPanelVideo.panelHidden && _gimbalController ? _gimbalController.gimbals.count : false

                    onVisibleChanged: {
                        checked = false
                    }
                }
            ]
        }
    }

    ToolStrip {
        id:        modesToolStrip
        width:     toolStripPanelVideo.height
        maxHeight: width * 5
        visible:   rootItem._modesPanelVisible
        model:     modesToolStripActionList.model
        fontSize:  ScreenTools.isMobile ? ScreenTools.smallFontPointSize * 0.7 : ScreenTools.smallFontPointSize

        anchors.top:                toolStripPanelVideo.bottom
        anchors.left:               toolStripPanelVideo.left
        anchors.leftMargin:         toolStripPanelVideo.height
        anchors.topMargin:          _margins

        ToolStripActionList {
            id: modesToolStripActionList
            model: [
                ToolStripAction {
                    text:               qsTr("RC target")
                    iconSource:         "/HA_Icons/PAYLOAD.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.gimbalController.setGimbalRcTargeting() : undefined
                },
                ToolStripAction {
                    text:               qsTr("Yaw Lock")
                    iconSource:         "/HA_Icons/YAW_LOCK.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.gimbalController.toggleGimbalYawLock(true, true) : undefined
                },
                ToolStripAction {
                    text:               qsTr("Yaw Follow")
                    iconSource:         "/HA_Icons/YAW_UNLOCK.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.gimbalController.toggleGimbalYawLock(true, false) : undefined
                },
                ToolStripAction {
                    text:               qsTr("Retract")
                    iconSource:         "/HA_Icons/RETRACT_ON.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.gimbalController.toggleGimbalRetracted(true, true) : undefined
                },
                ToolStripAction {
                    text:               qsTr("Neutral")
                    iconSource:         "/HA_Icons/NEUTRAL.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.gimbalController.toggleGimbalNeutral(true, true) : undefined
                }
            ]
        }
    }

    Rectangle {
        id:        actionsPanel
        height:    toolStripPanelVideo.height * 3
        width:     _actionsMapPanelVisible ? height * 2 : toolStripPanelVideo.height
        color:     qgcPal.toolbarBackground
        radius:    ScreenTools.defaultFontPixelWidth / 2
        visible:   rootItem._actionsPanelVisible

        anchors.top:                toolStripPanelVideo.bottom
        anchors.left:               modesToolStrip.right
        anchors.topMargin:          _margins


        // Buttons for tilt 90 and point to home
        ToolStrip {
            id:        actionsToolStrip
            width:     toolStripPanelVideo.height
            maxHeight: parent.height
            model:     actionsToolStripActionList.model
            fontSize:  ScreenTools.isMobile ? ScreenTools.smallFontPointSize * 0.7 : ScreenTools.smallFontPointSize

            anchors.top:                parent.top
            anchors.left:               parent.left

            ToolStripActionList {
                id: actionsToolStripActionList
                model: [
                    ToolStripAction {
                        text:               qsTr("Tilt 90")
                        iconSource:         "/HA_Icons/CAMERA_90.png"
                        onTriggered: {
                            if (_activeVehicle) {
                                _activeVehicle.gimbalController.sendPitchBodyYaw(-90, 0) // point gimbal down
                            }
                        }
                    },
                    ToolStripAction {
                        text:               qsTr("Point Home")
                        iconSource:         "/HA_Icons/HOME.png"
                        onTriggered:        _activeVehicle ? _activeVehicle.gimbalController.setGimbalHomeTargeting() : undefined
                    },
                    ToolStripAction {
                        id:                 mapToolsToolStripAction
                        text:               qsTr("Map tools")
                        iconSource:         "/HA_Icons/MAP_CLICK.png"
                        checkable:          true
                        visible:            !toolStripPanelVideo.panelHidden
                        enabled:            flightView._mainWindowIsMap

                        onVisibleChanged: {
                            if (!visible)
                                checked = false
                        }
                    }
                ]
            }
        }

        Rectangle {
            id:      gimbalMapActions
            color:   qgcPal.window
            radius:  ScreenTools.defaultFontPixelWidth / 2
            visible: rootItem._actionsMapPanelVisible && rootItem._actionsPanelVisible

            anchors.left:   actionsToolStrip.right
            anchors.right:  parent.right
            anchors.top:    parent.top
            anchors.bottom: parent.bottom

            onVisibleChanged: {
                getFromMapButton.checked = false
            }

            DeadMouseArea {
                anchors.fill: parent
            }

            QGCLabel {
                id: gimbalMapActionsLabel
                text: qsTr("Map ROI targetting tools")
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top:              parent.top
                anchors.margins:          _margins
                font.pointSize:           ScreenTools.smallFontPointSize
            }

            // Left grid, coordinates
            GridLayout {
                id:             gimbalMapActionsGridLeft
                anchors.top:    gimbalMapActionsLabel.bottom
                anchors.left:   parent.left
                anchors.right:  parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.margins:_margins

                columnSpacing:  _margins
                rowSpacing:     _margins
                columns:        2

                QGCLabel {
                    text: qsTr("Lat")
                    font.pointSize: ScreenTools.smallFontPointSize
                }
                FactTextField {
                    fact:               _activeVehicle ? _activeVehicle.gimbalTargetSetLatitude : null
                    font.pointSize:     ScreenTools.smallFontPointSize
                    implicitHeight:     ScreenTools.defaultFontPixelHeight
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    text: qsTr("Long")
                    font.pointSize: ScreenTools.smallFontPointSize

                }
                FactTextField {
                    fact:               _activeVehicle ? _activeVehicle.gimbalTargetSetLongitude : null
                    font.pointSize:     ScreenTools.smallFontPointSize
                    implicitHeight:     ScreenTools.defaultFontPixelHeight
                    Layout.fillWidth:   true
                }
                QGCLabel {
                    text:                   qsTr("Altitude above home(m)")
                    wrapMode:               Text.Wrap
                    maximumLineCount:       2
                    elide:                  Text.ElideRight
                    font.pointSize:         ScreenTools.smallFontPointSize
                    Layout.columnSpan:      2
                }
                QGCLabel {
                    text: qsTr("Alt")
                    font.pointSize: ScreenTools.smallFontPointSize
                }
                FactTextField {
                    fact:               _activeVehicle ? _activeVehicle.gimbalTargetSetAltitude : null
                    font.pointSize:     ScreenTools.smallFontPointSize
                    implicitHeight:     ScreenTools.defaultFontPixelHeight
                    Layout.fillWidth:   true
                }
                QGCButton {
                    id:                getFromMapButton
                    text:              qsTr("Get from map")
                    checkable:         true
                    Layout.columnSpan: 2
                    Layout.fillWidth:  true
                    pointSize:         ScreenTools.smallFontPointSize
                    implicitHeight:    ScreenTools.implicitButtonHeight * 0.6
                    backRadius:        ScreenTools.defaultFontPixelWidth / 2

                    onCheckedChanged: {
                        if (_activeVehicle) {
                            _activeVehicle.GimbalClickOnMapActive = checked
                        }
                    }
                }
            }

            // Right grid, options
            GridLayout {
                id:             gimbalMapActionsGridRight
                anchors.top:    gimbalMapActionsLabel.bottom
                anchors.left:   gimbalMapActionsGridLeft.right
                anchors.right:  parent.right
                anchors.bottom: parent.bottom
                anchors.margins:_margins

                columnSpacing:  _margins
                rowSpacing:     _margins
                columns:        2

                property var roiActive: _activeVehicle && _activeVehicle.isROIEnabled ? true : false

                QGCButton {
                    text:               qsTr("Send")
                    Layout.columnSpan:  2
                    Layout.alignment:   Qt.AlignHCenter | Qt.AlignTop
                    checkable:          false
                    backRadius:         ScreenTools.defaultFontPixelWidth / 2
                    pointSize:          ScreenTools.smallFontPointSize
                    implicitHeight:     ScreenTools.implicitButtonHeight * 0.6


                    onClicked: {
                        var coordinate = QtPositioning.coordinate(_activeVehicle.gimbalTargetSetLatitude.rawValue, _activeVehicle.gimbalTargetSetLongitude.rawValue, _activeVehicle.gimbalTargetSetAltitude.rawValue)
                        _activeVehicle.guidedModeROI(coordinate)
                    }
                }

                QGCLabel {
                    text:                   qsTr("Roi Active:")
                    wrapMode:               Text.Wrap
                    maximumLineCount:       2
                    elide:                  Text.ElideRight
                    font.pointSize:         ScreenTools.smallFontPointSize
                    visible:                gimbalMapActionsGridRight.roiActive
                    Layout.fillWidth:       true
                }

                QGCButton {
                    text:             qsTr("Cancel")
                    visible:          gimbalMapActionsGridRight.roiActive
                    pointSize:        ScreenTools.smallFontPointSize
                    implicitHeight:   ScreenTools.implicitButtonHeight * 0.6
                    backRadius:       ScreenTools.defaultFontPixelWidth / 2
                    Layout.fillWidth: true

                    onPressed: {
                        if (_activeVehicle) {
                            _activeVehicle.stopGuidedModeROI()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id:        gimbalSelectorPanel
        width:     toolStripPanelVideo.height
        height:    panelHeight
        visible:   rootItem._selectPanelVisible
        color:     qgcPal.windowShade
        radius:    ScreenTools.defaultFontPixelWidth / 2

        anchors.top:                toolStripPanelVideo.bottom
        anchors.right:              toolStripPanelVideo.right
        anchors.topMargin:          _margins

        property var buttonWidth:    width - _margins * 2
        property var panelHeight:    gimbalSelectorContentGrid.childrenRect.height + _margins * 2
        property var gridRowSpacing: _margins
        property var buttonFontSize: ScreenTools.smallFontPointSize * 0.9

        GridLayout {
            id:               gimbalSelectorContentGrid
            width:            parent.width
            rowSpacing:       gimbalSelectorPanel.gridRowSpacing
            columns:          1

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top:              parent.top
            anchors.topMargin:        _margins

            Repeater {
                model: _gimbalController && _gimbalController.gimbals ? _gimbalController.gimbals : undefined
                delegate: FakeToolStripHoverButton {
                    Layout.preferredWidth:  gimbalSelectorPanel.buttonWidth
                    Layout.preferredHeight: Layout.preferredWidth
                    Layout.alignment:       Qt.AlignHCenter | Qt.AlignVCenter
                    text:                   qsTr("Gimbal ") + object.deviceId
                    fontPointSize:          gimbalSelectorPanel.buttonFontSize
                    onClicked: {
                        _gimbalController.activeGimbal = object
                    }
                }
            }
        }
    }
}
