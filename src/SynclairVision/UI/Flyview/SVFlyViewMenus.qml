import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

import "SVFlyViewMenusList.js" as SVFlyViewMenusList

Item {
    id: root

    property real leftToolStripBottom: 0
    property string activeSettingsId: ""
    property var settingsModel: []
    property bool uiInteractionEnabled: false
    property real controlPanelRight: 0
    property real pipViewWidth: 0
    property var visibleCameraSlots: []
    property bool cursorTargetingAvailable: false

    signal settingsSelected(string settingsId)
    signal layoutSelected(string layoutId)
    signal trackingSelected(string trackingId)

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool digiviewActive: SVState.digiviewActive
    readonly property bool hasCurrentVideoOutputParameters: digiviewActive
        && digiview.hasVideoOutputParameters
        && digiview.videoOutputStreamName === digiview.streamName

    function setLayout(layoutId) {
        if (!root.digiviewActive) {
            return
        }

        var layoutModel = SVFlyViewMenusList.getLayoutModel(root.uiInteractionEnabled, DigiviewProtocol)
        var layoutMode = -1
        var i

        for (i = 0; i < layoutModel.length; i++) {
            if (layoutModel[i].id === layoutId) {
                layoutMode = layoutModel[i].value
                break
            }
        }

        if (layoutMode < 0) {
            return
        }

        digiview.setVideoOutputLayout(layoutMode)
    }

    function activateTrackingMode(trackingId) {
        if (trackingId === "coordsTrack") {
            if (SVState.activateManualTracking()) {
                root.trackingSelected("manual")
            }
            return
        }

        if ((trackingId === "cursorTrack" || trackingId === "singleTarget")
                && (!root.cursorTargetingAvailable
                    || !SVState.beginPointTrackingSelection(
                        trackingId, SVState.cameraSelected, root.visibleCameraSlots))) {
            return
        }

        root.trackingSelected(trackingId)
    }

    Connections {
        target: SVState

        function onCursorTrackingSelectionCancelled() {
            root.trackingSelected("")
        }
    }

    QGCPopupDialogFactory {
        id: switchTrackingModeDialogFactory

        dialogComponent: switchTrackingModeDialogComponent
    }

    Component {
        id: switchTrackingModeDialogComponent

        QGCSimpleMessageDialog {
            property string trackingId: ""

            title: qsTr("Switch tracking mode?")
            text: qsTr("Are you sure you want to switch tracking modes?")
            buttons: Dialog.Yes | Dialog.No

            onAccepted: root.activateTrackingMode(trackingId)
        }
    }

    SVMenuStrip {
        id: settingsMenu
        anchors.top: parent.top
        anchors.right: parent.right
        visible: SVState.hud && SVState.cursorTrackingSelect

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
    
        visible: SVState.hud && SVState.cursorTrackingSelect

        exclusiveSelection: false
        autoUpdateActiveId: false
        activeIds: SVState.aiOverlay ? ["aiOverlay"] : []

        model: [
            { 
                id: "aiOverlay",
                text: "Detect",
                description: "Show/Hide AI Detection Overlay",
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
        }
    }

    SVMenuStrip {
        id: tracking
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: pipViewWidth + SVUnits.margin
        enabled: root.uiInteractionEnabled
        visible: SVState.hud && SVState.cursorTrackingSelect

        menuText: "Tracking"
        menuDescription: "Settings for Tracking"

        source: "/qmlimages/tracking_main.svg"
        direction: horizontal
        isLeft: true
        isTop: false
        open: false

        autoUpdateActiveId: false
        activeId: SVState.activeCameraTrackingId

        model: SVFlyViewMenusList.getTrackingModel(SVState.hasActiveCamera && root.uiInteractionEnabled)

        onItemSelected: (id) => {
            const trackingStateId = id === "coordsTrack" ? "manual" : id
            if (SVState.activeCameraTrackingId === "") {
                root.activateTrackingMode(id)
                return
            }

            if (SVState.activeCameraTrackingId === trackingStateId) {
                if (SVState.stopActiveTracking()) {
                    root.trackingSelected("")
                }
                return
            }

            switchTrackingModeDialogFactory.open({ trackingId: id })
        }
    }

    SVMenuStrip {
        id: layout
        anchors.top: aiDetectionOverlay.bottom
        anchors.topMargin: SVUnits.margin
        anchors.left: parent.left
        enabled: root.uiInteractionEnabled
        visible: SVState.hud && SVState.cursorTrackingSelect

        menuText: "Layout"
        menuDescription: "Camera Layouts"

        source: "/qmlimages/layout_main.svg"
        direction: vertical
        isLeft: true
        isTop: true
        open: false
        autoUpdateActiveId: false
        activeId: {
            if (!root.hasCurrentVideoOutputParameters) {
                return ""
            }

            const layoutMode = root.digiview.videoOutputLayoutMode
            const layoutModel = SVFlyViewMenusList.getLayoutModel(root.uiInteractionEnabled, DigiviewProtocol)
            for (let i = 0; i < layoutModel.length; ++i) {
                if (layoutModel[i].value === layoutMode) {
                    return layoutModel[i].id
                }
            }

            return ""
        }

        model: SVFlyViewMenusList.getLayoutModel(root.uiInteractionEnabled, DigiviewProtocol)

        onItemSelected: (id) => {
            if (!root.uiInteractionEnabled) {
                return
            }

            SVState.cameraSelected = -1
            root.layoutSelected(id)
            root.setLayout(id)
        }
    }

    SVMenuStrip {
        id: overlays
        anchors.bottom: parent.bottom
        anchors.left: lockButton.right
        anchors.leftMargin: SVUnits.margin
        visible: SVState.hud && SVState.cursorTrackingSelect
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
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: parent.width / 4
        anchors.bottom: parent.bottom
        visible: SVState.hud && SVState.cursorTrackingSelect
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
