import QtQml.Models                 2.12
import QtQuick                      2.12
import QtQuick.Controls             2.15
import QtQuick.Layouts              1.11
import QtQuick.Dialogs              1.2
import QtPositioning                5.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactControls  1.0

Item {
    id:     rootItem
    width:  toolStripPanelVideo.width
    height: toolStripPanelVideo.height

    property alias maxWidth:                toolStripPanelVideo.maxWidth
    property real  _margins:                ScreenTools.defaultFontPixelWidth * 0.75
    property bool  _modesPanelVisible:      modesToolStripAction.checked
    property bool  _actionsPanelVisible:    actionsToolStripAction.checked
    property bool  _selectPanelVisible:     selectToolStripAction.checked
    property bool  _actionsMapPanelVisible: mapToolsToolStripAction.checked
    property var   _activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property bool  _haveGimbalControl:      _activeVehicle ? _activeVehicle.gimbalHaveControl : false
    property bool  _othersHaveGimbalControl: _activeVehicle ? _activeVehicle.gimbalOthersHaveControl : false

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
                    onTriggered:       mainWindow.showFlyView()
                    checkable:         true
                    visible:           !toolStripPanelVideo.panelHidden

                    onVisibleChanged: {
                        checked = false
                    }
                },
                ToolStripAction {
                    id:                 selectToolStripAction
                    text:               qsTr("")
                    iconSource:         "/HA_Icons/SELECT.png"
                    checkable:          true
                    visible:            !toolStripPanelVideo.panelHidden
                    
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
                    onTriggered:        _activeVehicle ? _activeVehicle.setGimbalRcTargeting() : undefined
                },
                ToolStripAction {
                    text:               qsTr("Yaw Lock")
                    iconSource:         "/HA_Icons/YAW_LOCK.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.toggleGimbalYawLock(true, true) : undefined
                },
                ToolStripAction {
                    text:               qsTr("Yaw Follow")
                    iconSource:         "/HA_Icons/YAW_UNLOCK.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.toggleGimbalYawLock(true, false) : undefined
                },
                ToolStripAction {
                    text:               qsTr("Retract")
                    iconSource:         "/HA_Icons/RETRACT_ON.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.toggleGimbalRetracted(true, true) : undefined
                },
                ToolStripAction {
                    text:               qsTr("Neutral")
                    iconSource:         "/HA_Icons/NEUTRAL.png"
                    onTriggered:        _activeVehicle ? _activeVehicle.toggleGimbalNeutral(true, true) : undefined
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
                                if (_activeVehicle.gimbalOthersHaveControl) {
                                     // TODO: we should mention who is currently in control
                                     mainWindow.showMessageDialog(title,
                                         qsTr("Do you want to take over gimbal control?"),
                                         StandardButton.Yes | StandardButton.Cancel,
                                         function() {
                                            _activeVehicle.acquireGimbalControl()
                                            _activeVehicle.toggleGimbalYawLock(true, false) // we need yaw lock for this
                                            _activeVehicle.sendGimbalManagerPitchYaw(0, -90) // point gimbal down
                                         })
                                } else if (!_activeVehicle.othersHaveControl) {
                                    _activeVehicle.acquireGimbalControl()
                                    _activeVehicle.toggleGimbalYawLock(true, false) // we need yaw lock for this
                                    _activeVehicle.sendGimbalManagerPitchYaw(0, -90) // point gimbal down
                                } else {
                                    _activeVehicle.toggleGimbalYawLock(true, false) // we need yaw lock for this
                                    _activeVehicle.sendGimbalManagerPitchYaw(0, -90) // point gimbal down
                                }
                            }
                        }
                    },
                    ToolStripAction {
                        text:               qsTr("Point Home")
                        iconSource:         "/HA_Icons/HOME.png"
                        onTriggered:        _activeVehicle ? _activeVehicle.setGimbalHomeTargeting() : undefined
                    },
                    ToolStripAction {
                        id:                 mapToolsToolStripAction
                        text:               qsTr("Map tools")
                        iconSource:         "/HA_Icons/MAP_CLICK.png"
                        checkable:          true
                        visible:            !toolStripPanelVideo.panelHidden

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

    ToolStrip {
        id:        selectToolStrip
        width:     toolStripPanelVideo.height
        maxHeight: width * 2
        visible:   rootItem._selectPanelVisible
        model:     selectToolStripActionList.model
        fontSize:  ScreenTools.isMobile ? ScreenTools.smallFontPointSize * 0.7 : ScreenTools.smallFontPointSize

        anchors.top:                toolStripPanelVideo.bottom
        anchors.right:              toolStripPanelVideo.right
        anchors.topMargin:          _margins

        ToolStripActionList {
            id: selectToolStripActionList
            model: [
                ToolStripAction {
                    text:               qsTr("Gimbal 1")
                    iconSource:         "/HA_Icons/PAYLOAD.png"
                    onTriggered:        undefined
                },
                ToolStripAction {
                    text:               qsTr("Gimbal 2")
                    iconSource:         "/HA_Icons/PAYLOAD.png"
                    onTriggered:        undefined
                }
            ]
        }
    }
}
