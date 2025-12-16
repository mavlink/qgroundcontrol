import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.UTMSP

Item {
    required property var editorMap
    required property var planMasterController

    id: root

    // These must match the indices of _editingToolComponents
    readonly property int _editingToolMission:  0
    readonly property int _editingToolFence:    1
    readonly property int _editingToolRally:    2

    property int _editingTool:              _editingToolMission
    property var _missionController:        planMasterController.missionController
    property var _geoFenceController:       planMasterController.geoFenceController
    property var _rallyPointController:     planMasterController.rallyPointController
    property var _visualItems:              _missionController.visualItems
    property var _editingToolComponents:    [ missionToolComponent, fenceToolComponent, rallyToolComponent ]
    property real _toolsMargin:             ScreenTools.defaultFontPixelWidth * 0.75

    function selectNextNotReady() {
        var foundCurrent = false
        for (var i=0; i<_missionController.visualItems.count; i++) {
            var vmi = _missionController.visualItems.get(i)
            if (vmi.readyForSaveState === VisualMissionItem.NotReadyForSaveData) {
                _missionController.setCurrentPlanViewSeqNum(vmi.sequenceNumber, true)
                break
            }
        }
    }

    QGCPalette { id: qgcPal }

    Rectangle {
        id:             rightPanelBackground
        anchors.fill:   parent
        color:          qgcPal.window
        opacity:        0.85
    }


    // Open/Close panel
    Item {
        id:                     panelOpenCloseButton
        anchors.right:          parent.left
        anchors.verticalCenter: parent.verticalCenter
        width:                  toggleButtonRect.width - toggleButtonRect.radius
        height:                 toggleButtonRect.height
        clip:                   true

        property bool _expanded: root.anchors.right == root.parent.right

        Rectangle {
            id:             toggleButtonRect
            width:          ScreenTools.defaultFontPixelWidth * 2.25
            height:         width * 3
            radius:         ScreenTools.defaultBorderRadius
            color:          rightPanelBackground.color
            opacity:        rightPanelBackground.opacity

            QGCLabel {
                id:                 toggleButtonLabel
                anchors.centerIn:   parent
                text:               panelOpenCloseButton._expanded ? ">" : "<"
                color:              qgcPal.buttonText
            }

        }

        QGCMouseArea {
            anchors.fill: parent

            onClicked: {
                if (panelOpenCloseButton._expanded) {
                    // Close panel
                    root.anchors.right = undefined
                    root.anchors.left = root.parent.right
                } else {
                    // Open panel
                    root.anchors.left = undefined
                    root.anchors.right = root.parent.right
                }
            }
        }
    }

    //-------------------------------------------------------
    // Right Panel Controls
    Item {
        anchors.fill:           rightPanelBackground
        anchors.topMargin:      _toolsMargin

        DeadMouseArea {
            anchors.fill:   parent
        }

        ColumnLayout {
            anchors.fill:   parent
            spacing:        ScreenTools.defaultFontPixelHeight * 0.5

            QGCTabBar {
                Layout.fillWidth: true

                QGCTabButton {
                    text:       qsTr("Mission")
                    onClicked:  { root._editingTool = root._editingToolMission; _editingLayer = _layerMission }
                }

                QGCTabButton {
                    text:       qsTr("Fence")
                    onClicked:  { root._editingTool = root._editingToolFence; _editingLayer = _layerFence }
                }

                QGCTabButton {
                    text:       qsTr("Rally")
                    onClicked:  { root._editingTool = root._editingToolRally; _editingLayer = _layerRally }
                }
            }

            Loader {
                id:                 editingToolLoader
                Layout.fillWidth:   true
                Layout.fillHeight:  true
                sourceComponent:    root._editingToolComponents[root._editingTool]
            }
        }

        Component {
            id: missionToolComponent

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                GridLayout {
                    Layout.margins: _toolsMargin
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: _toolsMargin
                    rowSpacing: _toolsMargin
                    visible: !planMasterController.containsItems

                    QGCLabel {
                        Layout.columnSpan: 2
                        text: qsTr("Create from template")
                    }

                    Repeater {
                        model: planMasterController.planCreators

                        Rectangle {
                            id: planCreatorButton
                            Layout.fillWidth: true
                            implicitHeight: planCreatorLayout.implicitHeight
                            color: planCreatorButtonMouseArea.pressed || planCreatorButtonMouseArea.containsMouse ? qgcPal.buttonHighlight : qgcPal.button

                            ColumnLayout {
                                id: planCreatorLayout
                                width: parent.width
                                spacing: 0

                                Image {
                                    id: planCreatorImage
                                    Layout.fillWidth: true
                                    source: object.imageResource
                                    sourceSize.width: width
                                    fillMode: Image.PreserveAspectFit
                                    mipmap: true
                                }

                                QGCLabel {
                                    id: planCreatorNameLabel
                                    Layout.fillWidth: true
                                    Layout.maximumWidth: parent.width
                                    horizontalAlignment: Text.AlignHCenter
                                    text: object.name
                                    color: planCreatorButtonMouseArea.pressed || planCreatorButtonMouseArea.containsMouse ? qgcPal.buttonHighlightText : qgcPal.buttonText
                                }
                            }

                            QGCMouseArea {
                                id: planCreatorButtonMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                preventStealing: true
                                onClicked: object.createPlan(_mapCenter())

                                function _mapCenter() {
                                    var centerPoint = Qt.point(editorMap.centerViewport.left + (editorMap.centerViewport.width / 2), editorMap.centerViewport.top + (editorMap.centerViewport.height / 2))
                                    return editorMap.toCoordinate(centerPoint, false /* clipToViewPort */)
                                }
                            }
                        }
                    }
                }

                QGCListView {
                    id:                 missionItemEditorListView
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true
                    spacing:            ScreenTools.defaultFontPixelHeight / 4
                    orientation:        ListView.Vertical
                    model:              _missionController.visualItems
                    cacheBuffer:        Math.max(height * 2, 0)
                    clip:               true
                    currentIndex:       _missionController.currentPlanViewSeqNum
                    highlightMoveDuration: 250

                    delegate: MissionItemEditor {
                        map:                editorMap
                        masterController:   planMasterController
                        missionItem:        object
                        width:              missionItemEditorListView.width
                        readOnly:           false

                        onClicked: _missionController.setCurrentPlanViewSeqNum(object.sequenceNumber, false)

                        onRemove: {
                            var removeVIIndex = index
                            _missionController.removeVisualItem(removeVIIndex)
                            if (removeVIIndex >= _missionController.visualItems.count) {
                                removeVIIndex--
                            }
                        }

                        onSelectNextNotReadyItem: selectNextNotReady()
                    }
                }
            }
        }

        Component {
            id: fenceToolComponent

            Column {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                GeoFenceEditor {
                    width:                  parent.width
                    myGeoFenceController:   root._geoFenceController
                    flightMap:              root.editorMap
                }
            }
        }


        Component {
            id: rallyToolComponent

            Column {
                spacing: ScreenTools.defaultFontPixelHeight / 2

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
            }
        }

        Component {
            id: utmspToolComponent

            Column {
                spacing: ScreenTools.defaultFontPixelHeight / 2

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
