import QtQuick

import QGroundControl
import QGroundControl.Controls

import "SVFlyViewMenusList.js" as SVFlyViewMenusList

Item {
    id: root

    property real leftToolStripBottom: 0
    property string activeLayoutId: "four_square"
    property string activeTrackingId: ""
    property string activeSettingsId: ""
    property var settingsModel: []
    property bool uiInteractionEnabled: false
    property real controlPanelRight: 0
    property var pipViewWidth

    signal settingsSelected(string settingsId)
    signal layoutSelected(string layoutId)
    signal trackingSelected(string trackingId)

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool digiviewActive: SVState.digiviewActive

    function setLayout(layoutId) {
        if (!root.digiviewActive) {
            return
        }

        var layoutModel = SVFlyViewMenusList.getLayoutModel(root.uiInteractionEnabled)
        var layoutMode = -1
        var i

        for (i = 0; i < layoutModel.length; i++) {
            if (layoutModel[i].id === layoutId) {
                layoutMode = i
                break
            }
        }

        if (layoutMode < 0) {
            return
        }

        digiview.sendSetVideoOutput(
            "stream",
            SVSettings.videoResolutionWidth,
            SVSettings.videoResolutionHeight,
            SVSettings.videoFps,
            layoutMode,
            0
        )
    }

    SVMenuStrip {
        id: settingsMenu
        anchors.top: parent.top
        anchors.right: parent.right
        visible: SVState.hud

        menuText: "Settings"
        menuDescription: "Settings"
        source: "/qmlimages/settings_main.svg"
        alternateSource: "/qmlimages/settings_main_open.svg"
        direction: vertical
        menuDirection: horizontal
        isLeft: false
        isTop: true
        open: false
        autoUpdateActiveId: false
        activeId: root.activeSettingsId

        model: root.settingsModel

        onItemSelected: (id) => root.settingsSelected(id)
    }

    SVMenuStrip {
        id: oneshots
        headerless: true
        exclusiveSelection: false
        autoUpdateActiveId: false
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 7 + ScreenTools.defaultFontPixelWidth * 0.5

        direction: horizontal
        isLeft: false
        isTop: true

        activeIds: {
            var ids = []

            if (!SVState.hud) {
                ids.push("hud")
            }

            if (SVState.record) {
                ids.push("record")
            }

            if (!SVState.toolbar) {
                ids.push("toolbar")
            }

            return ids
        }

        model: SVFlyViewMenusList.getOneShotModel(root.uiInteractionEnabled)

        onItemSelected: (id) => {
            if (id === "hud") {
                SVState.toggleHud()
                return
            }

            if (id === "photo") {
                if (!root.uiInteractionEnabled) {
                    return
                }

                SVState.takePhoto()
                return
            }

            if (id === "record") {
                if (!root.uiInteractionEnabled) {
                    return
                }

                SVState.toggleRecord()
                return
            }

            if (id === "toolbar") {
                SVState.toggleToolbar()
                return
            }
        }
    }

    SVMenuStrip {
        id: aiDetectionOverlay
        headerless: true
        anchors.top: parent.top
        anchors.topMargin: root.leftToolStripBottom + SVUnits.margin
        anchors.left: parent.left
        isLeft: true
        isTop: true
    
        visible: SVState.hud

        exclusiveSelection: false
        autoUpdateActiveId: false
        activeIds: SVState.aiOverlay ? ["aiOverlay"] : []

        model: [
            { 
                id: "aiOverlay",
                text: "Overlay",
                description: "Show/Hide AI Overlay",
                checkable: true,
                iconSource: "/qmlimages/layout_ai.svg",
                alternateIconSource: "/qmlimages/layout_ai_bold.svg",
                iconActive: SVState.aiOverlay,
                enabled: root.uiInteractionEnabled
            }
        ]

        onItemSelected: (id) => {
            if (!root.uiInteractionEnabled) {
                return
            }

            SVState.toggleAiOverlay()
            return
        }
    }

    SVMenuStrip {
        id: tracking
        anchors.bottom: parent.bottom
        anchors.left: pipViewWidth
        enabled: root.uiInteractionEnabled
        visible: SVState.hud

        menuText: "Tracking"
        menuDescription: "Settings for Tracking"

        source: "/qmlimages/layout_main.svg"
        direction: horizontal
        isLeft: true
        isTop: false
        open: false

        autoUpdateActiveId: false
        activeId: root.activeTrackingId

        model: SVFlyViewMenusList.getTrackingModel(SVState.cameraSelected !== -1 && root.uiInteractionEnabled)

        onItemSelected: (id) => {
            if(root.activeTrackingId === id) {
                root.activeTrackingId === -1
            } else {
                root.activeTrackingId = id
            }
            root.trackingSelected(id)
        }
    }

    SVMenuStrip {
        id: layout
        anchors.top: aiDetectionOverlay.bottom
        anchors.topMargin: SVUnits.margin
        anchors.left: parent.left
        enabled: root.uiInteractionEnabled
        visible: SVState.hud

        menuText: "Layout"
        menuDescription: "Camera Layouts"

        source: "/qmlimages/layout_main.svg"
        direction: vertical
        isLeft: true
        isTop: true
        open: false
        autoUpdateActiveId: false
        activeId: root.activeLayoutId

        model: SVFlyViewMenusList.getLayoutModel(root.uiInteractionEnabled)

        onItemSelected: (id) => {
            if (!root.uiInteractionEnabled) {
                return
            }

            SVState.cameraSelected = -1
            root.layoutSelected(id)
            root.setLayout(id);
        }
    }

    SVMenuStrip {
        id: overlays
        anchors.bottom: parent.bottom
        anchors.left: lockButton.right
        anchors.leftMargin: SVUnits.margin
        visible: SVState.hud
        enabled: root.uiInteractionEnabled
        direction: horizontal
        isLeft: false
        isTop: false

        exclusiveSelection: false
        autoUpdateActiveId: false
        open: false

        menuText: "Overlay"
        menuDescription: "Change Overlay Elements"
        source: "/qmlimages/overlay_main.svg"

        activeIds: {
            var ids = []

            if (SVState.grid) {
                ids.push("grid")
            }

            if (SVState.crosshair) {
                ids.push("crosshair")
            }
            return ids
        }

        model: SVFlyViewMenusList.getOverlaysModel(SVState.cameraSelected !== -1 && root.uiInteractionEnabled)

        onItemSelected: (id) => {
            if (id === "grid") {
                SVState.toggleGrid()
                return
            }

            if (id === "crosshair") {
                SVState.toggleCrosshair()
                return
            }
        }

         
        
    }
    

    SVMenuStrip {
        id: lockButton
        headerless: true
        anchors.left: parent.left
        anchors.leftMargin: root.controlPanelRight
        anchors.bottom: parent.bottom
        visible: SVState.hud
        direction: horizontal
        isLeft: false
        isTop: false

        exclusiveSelection: false
        autoUpdateActiveId: false
        activeIds: SVState.lockControls ? ["lock"] : []

        model: [
            { 
                id: "lock",
                text: "Lock",
                description: "Lock/Unlock Controls",
                checkable: true,
                iconSource: "/qmlimages/controls_lock.svg",
                alternateIconSource: "/qmlimages/controls_lock_closed.svg",
                iconActive: SVState.lockControls,
                enabled: true
            }
        ]

        onItemSelected: (id) => {
            SVState.toggleLockControls()
            return
        }
    }
}
