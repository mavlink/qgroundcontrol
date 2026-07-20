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

    property real leftToolStripBottom
    property string activeLayoutId: "four_square"
    property string activeSettingsId: ""
    readonly property var settingsModel: SVFlyViewMenusList.getSettingsModel()

    property bool uiInteractionEnabled: SVState.uiInteractionEnabled

    signal layoutSelected(string layoutId)

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

        // horizontal: right-aligned for Bottom-right, otherwise centered
        anchors.right: isBottomRight ? parent.right : undefined
        anchors.left: isBottomRight ? undefined : parent.horizontalCenter
        anchors.leftMargin: isBottomRight ? 0 : -SVSettings.joystickSize / 8 * SVUnits.height

        // vertical: top for Top-center, otherwise bottom
        anchors.top: isTopCenter ? parent.top : undefined
        anchors.bottom: !isTopCenter ? parent.bottom : undefined

        visible: SVState.hud && SVSettings.controlPanel
    }

    SVSettingsDrawer {
        id: settingsDrawer
        anchors.fill: parent

        activeSettingsId: root.activeSettingsId
        settingsModel: root.settingsModel

        onSettingsSelected: (settingsId) => root.activeSettingsId = settingsId
        onDismissed: root.activeSettingsId = ""
    }

    SVFlyViewMenus {
        anchors.fill: parent

        leftToolStripBottom: root.leftToolStripBottom
        activeLayoutId: root.activeLayoutId
        activeSettingsId: root.activeSettingsId
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

    SVRecordInfoBox {
        id: recordInfoBox
        anchors.left: parent.left
        anchors.leftMargin: SVUnits.margin + ((SVState.hud) ? SVUnits.objectWidth : 0)
        anchors.top: parent.top
        visible: SVState.record && SVSettings.recordInformationBox
    } 

    SVFlyViewStats {
        id: statsBox
        anchors.right: parent.right
        anchors.top: parent.verticalCenter
        open: true

    }

    
}
