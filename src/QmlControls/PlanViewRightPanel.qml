import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.UTMSP

pragma ComponentBehavior: Bound

Item {
    required property var editorMap
    required property var planMasterController

    id: root

    // These must match the indices of _editingToolComponents
    readonly property int _editingToolStart:        0
    readonly property int _editingToolMissionItem:  1
    readonly property int _editingToolOther:        2

    property int _editingTool:              _editingToolMissionItem
    property var _missionController:        planMasterController.missionController
    property var _geoFenceController:       planMasterController.geoFenceController
    property var _rallyPointController:     planMasterController.rallyPointController
    property var _visualItems:              _missionController.visualItems
    property var _editingToolComponents:    [ startToolComponent, missionItemToolComponent, otherToolComponent ]
    property real  _toolsMargin:            ScreenTools.defaultFontPixelWidth * 0.75

    QGCPalette { id: qgcPal }

    Rectangle {
        id:             rightPanelBackground
        anchors.fill:   parent
        color:          qgcPal.window
        opacity:        0.85
    }

    //-------------------------------------------------------
    // Right Panel Controls
    Item {
        anchors.fill:           rightPanelBackground
        anchors.topMargin:      _toolsMargin

        DeadMouseArea {
            anchors.fill:   parent
        }

        Column {
            id:                 rightControls
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        parent.top

            QGCTabBar {
                width: parent.width

                Component.onCompleted: currentIndex = 1

                QGCTabButton {
                    text:       qsTr("Start")
                    onClicked:  { root._editingTool = root._editingToolStart; _editingLayer = _layerMission }
                }

                QGCTabButton {
                    text:       qsTr("Mission")
                    onClicked:  { root._editingTool = root._editingToolMissionItem; _editingLayer = _layerMission }
                }

                QGCTabButton {
                    text:       qsTr("Other")
                    onClicked:  { root._editingTool = root._editingToolOther; _editingLayer = _layerOther }
                }
            }
        }

        QGCFlickable {
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.top:            rightControls.bottom
            anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
            anchors.bottom:         parent.bottom
            anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
            contentHeight:          editingToolLoader.height

            Loader {
                id:                 editingToolLoader
                width:              parent.width
                sourceComponent:    root._editingToolComponents[root._editingTool]
            }
        }

        Component {
            id: startToolComponent

            MissionItemEditor {
                width:              parent.width
                map:                root.editorMap
                masterController:   root.planMasterController
                missionItem:        root._missionController.visualItems.get(0)
            }
        }

        Component {
            id: missionItemToolComponent

            Column {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                Column {
                    width:      parent.width
                    spacing:    ScreenTools.defaultFontPixelHeight / 2
                    visible:    root._missionController.currentPlanViewVIIndex !== 0

                    RowLayout {
                        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        height: ScreenTools.defaultFontPixelHeight

                        QGCColoredImage {
                            Layout.fillHeight:      true
                            Layout.preferredWidth:  height
                            source:                 "/InstrumentValueIcons/backward.svg"
                            color:                  qgcPal.buttonText

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (root._missionController.currentPlanViewVIIndex > 1) {
                                        let prevItem = root._missionController.visualItems.get(root._missionController.currentPlanViewVIIndex - 1)
                                        root._missionController.setCurrentPlanViewSeqNum(prevItem.sequenceNumber, false)
                                    }
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth:       true
                            horizontalAlignment:    Text.AlignHCenter
                            text:                   root._missionController.currentPlanViewItem.commandName
                        }

                        QGCColoredImage {
                            Layout.fillHeight:      true
                            Layout.preferredWidth:  height
                            source:                 "/InstrumentValueIcons/forward.svg"
                            color:                  qgcPal.buttonText

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (root._missionController.currentPlanViewVIIndex < root._missionController.visualItems.count - 1) {
                                        let nextItem = root._missionController.visualItems.get(root._missionController.currentPlanViewVIIndex + 1)
                                        root._missionController.setCurrentPlanViewSeqNum(nextItem.sequenceNumber, false)
                                    }
                                }
                            }
                        }
                    }

                    MissionItemEditor {
                        width:                      parent.width
                        map:                        root.editorMap
                        masterController:           root.planMasterController
                        missionItem:                root._missionController.currentPlanViewItem
                        onRemove:                   root._missionController.removeVisualItem(root._missionController.currentPlanViewVIIndex)
                        onSelectNextNotReadyItem:   selectNextNotReady()
                    }
                }

                QGCLabel {
                    width:                  parent.width
                    horizontalAlignment:    Text.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                    wrapMode:               Text.WordWrap
                    text:                   qsTr("Use the tools on the left to add mission items to the plan.")
                    visible:                root._missionController.currentPlanViewVIIndex === 0
                }
            }
        }

        Component {
            id: otherToolComponent

            Column {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                GeoFenceEditor {
                    width:                  parent.width
                    myGeoFenceController:   root._geoFenceController
                    flightMap:              root.editorMap
                }

                RallyPointEditorHeader {
                    width:              parent.width
                    controller:         root._rallyPointController
                }

                RallyPointItemEditor {
                    width:              parent.width
                    visible:            root._rallyPointController.points.count
                    rallyPoint:         root._rallyPointController.currentRallyPoint
                    controller:         root._rallyPointController
                }

                UTMSPAdapterEditor{
                    width:                  parent.width
                    currentMissionItems:     root._visualItems
                    myGeoFenceController:    root._geoFenceController
                    flightMap:               root.editorMap
                    triggerSubmitButton:     _triggerSubmit
                    resetRegisterFlightPlan: _resetRegisterFlightPlan

                    onRemoveFlightPlanTriggered: {
                        root.planMasterController.removeAllFromVehicle()
                        root._missionController.setCurrentPlanViewSeqNum(0, true)
                        _resetRegisterFlightPlan = true
                    }

                    onResetGeofencePolygonTriggered: {
                        resetUTMSPGeoFenceTimer.start()
                    }

                    Timer {
                        id:             resetUTMSPGeoFenceTimer
                        interval:       2500
                        running:        false
                        repeat:         false
                        onTriggered:    _resetGeofencePolygon = true
                    }
                }
            }
        }
    }
}
