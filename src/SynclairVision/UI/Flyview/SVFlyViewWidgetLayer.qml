import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

import "SVFlyViewMenusList.js" as SVFlyViewMenusList

Item {
    id: root


    property var offsetX: 0
    property var offsetY: 0
    property real leftToolStripBottom
    property string activeSettingsId: ""
    property real pipViewWidth: 0
    property var visibleCameraSlots: []
    property bool cursorTargetingAvailable: false
    readonly property var settingsModel: SVFlyViewMenusList.getSettingsModel()

    property bool uiInteractionEnabled: SVState.uiInteractionEnabled

    signal layoutSelected(string layoutId)

    onVisibleChanged: {
        if (!visible) {
            activeSettingsId = ""
            mainWindow.closeIndicatorDrawer()
        }
    }

    Connections {
        target: SVState

        function onCursorTrackingSessionActiveChanged() {
            if (!SVState.cursorTrackingSessionActive) {
                return
            }

            root.activeSettingsId = ""
            mainWindow.closeIndicatorDrawer()
        }
    }

    Timer {
        id: recordElapsedTimer

        interval: 1000
        repeat: true
        running: SVState.record

        onRunningChanged: {
            if (running) {
                SVState.startRecordTimer()
                return
            }

            SVState.stopRecordTimer()
        }

        onTriggered: SVState.updateRecordElapsedText()
    }

    SVControlPanel {
        id: controlPanel
        readonly property string panelPosition: SVSettings.controlPanelPosition
        readonly property bool isBottomRight: panelPosition === "Bottom-right"
        readonly property bool isTopCenter: panelPosition === "Top-center"

        width: implicitWidth
        height: implicitHeight

        state: isBottomRight ? "bottomRight" : isTopCenter ? "topCenter" : "bottomCenter"

        states: [
            State {
                name: "bottomRight"

                AnchorChanges {
                    target: controlPanel
                    anchors.left: undefined
                    anchors.right: parent.right
                    anchors.top: undefined
                    anchors.bottom: parent.bottom
                }

                PropertyChanges {
                    target: controlPanel
                    anchors.leftMargin: 0
                }
            },
            State {
                name: "topCenter"

                AnchorChanges {
                    target: controlPanel
                    anchors.left: parent.horizontalCenter
                    anchors.right: undefined
                    anchors.top: parent.top
                    anchors.bottom: undefined
                }

                PropertyChanges {
                    target: controlPanel
                    anchors.leftMargin: -SVSettings.joystickSize / 8 * SVUnits.height
                }
            },
            State {
                name: "bottomCenter"

                AnchorChanges {
                    target: controlPanel
                    anchors.left: parent.horizontalCenter
                    anchors.right: undefined
                    anchors.top: undefined
                    anchors.bottom: parent.bottom
                }

                PropertyChanges {
                    target: controlPanel
                    anchors.leftMargin: -SVSettings.joystickSize / 8 * SVUnits.height
                }
            }
        ]

        visible: SVState.hud && SVSettings.controlPanel && SVState.cursorTrackingSelect
    }

    SVSettingsDrawer {
        id: settingsDrawer
        anchors.fill: parent
        visible: !SVState.cursorTrackingSessionActive

        activeSettingsId: root.activeSettingsId
        settingsModel: root.settingsModel
        offsetX: root.offsetX
        offsetY: root.offsetY

        onSettingsSelected: (settingsId) => root.activeSettingsId = settingsId
        onDismissed: root.activeSettingsId = ""
    }

    SVFlyViewMenus {
        anchors.fill: parent

        leftToolStripBottom: root.leftToolStripBottom
        activeSettingsId: root.activeSettingsId
        pipViewWidth: root.pipViewWidth
        visibleCameraSlots: root.visibleCameraSlots
        cursorTargetingAvailable: root.cursorTargetingAvailable
        settingsModel: root.settingsModel
        uiInteractionEnabled: root.uiInteractionEnabled
        controlPanelRight: controlPanel.x + controlPanel.width + SVUnits.objectWidth

        onSettingsSelected: (settingsId) => {
            root.activeSettingsId = (root.activeSettingsId === settingsId) ? "" : settingsId

            if (root.activeSettingsId !== "") {
                settingsDrawer.openDrawer()
            }
        }

        onLayoutSelected: (layoutId) => root.layoutSelected(layoutId)
    }

    SVNotificationBox {
        id: notificationBox
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: SVUnits.objectWidth + SVUnits.margin + SVUnits.lineWidth
    }

    SVRecordInfoBox {
        id: recordInfoBox
        anchors.left: parent.left
        anchors.leftMargin: SVUnits.margin + ((SVState.hud && SVState.cursorTrackingSelect) ? SVUnits.objectWidth : 0)
        anchors.top: parent.top
        visible: SVState.record && SVSettings.recordInformationBox && !SVState.cursorTrackingSessionActive
    } 

    SVFlyViewStats {
        id: statsBox
        anchors.right: parent.right
        anchors.top: parent.verticalCenter
        height: parent / 2
        open: true
        visible: false

    }

    
}
