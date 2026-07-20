import QtQuick

import QGroundControl
import QGroundControl.Controls

import "SVFlyViewMenusList.js" as SVFlyViewMenusList

Item {
    id: root

    property real leftToolStripBottom: 0
    property string activeLayoutId: "four_square"
    property string activeSettingsId: ""
    property var settingsModel: []
    property bool uiInteractionEnabled: false
    property real controlPanelRight: 0

    signal settingsSelected(string settingsId)
    signal layoutSelected(string layoutId)

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
        source: "/qmlimages/settings_main.svg"
        alternateSource: "/qmlimages/settings_main_open.svg"
        direction: vertical
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
    
        visible: SVState.hud

        exclusiveSelection: false
        autoUpdateActiveId: false
        activeIds: SVState.aiOverlay ? ["aiOverlay"] : []

        model: [
            { 
                id: "aiOverlay",
                text: "Overlay",
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
        id: layout
        anchors.top: aiDetectionOverlay.bottom
        anchors.topMargin: SVUnits.margin
        anchors.left: parent.left
        visible: SVState.hud

        menuText: "Layout"
        source: "/qmlimages/layout_main.svg"
        direction: vertical
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
        id: lockButton
        headerless: true
        anchors.left: parent.left
        anchors.leftMargin: root.controlPanelRight
        anchors.bottom: parent.bottom
        visible: SVState.hud

        exclusiveSelection: false
        autoUpdateActiveId: false
        activeIds: SVState.lockControls ? ["lock"] : []

        model: [
            { 
                id: "lock",
                text: "Lock",
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
