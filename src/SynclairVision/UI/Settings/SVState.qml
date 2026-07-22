pragma Singleton
import QtQuick

import QGroundControl

QtObject {
    id: root

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool digiviewActive: !!(digiview
        && digiview.connected
        && QGroundControl.videoManager.streaming)
    readonly property bool uiInteractionEnabled: digiviewActive || SVSettings.devBypassDisconnectedUiDisable
    readonly property bool cameraSelectionEnabled: uiInteractionEnabled

    signal takePhotoRequested()
    signal cursorTargetRequested(int cameraSlot, real normalizedX, real normalizedY)
    signal cursorTrackingSelectionCancelled()

    function beginCursorTrackingSelection(cameraSlot, visibleCameraSlots) {
        if (!cameraSelectionEnabled || cameraSlot !== cameraSelected
                || cameraSlot < 0 || cameraSlot >= cameraTrackingIds.length
                || !visibleCameraSlots || visibleCameraSlots.indexOf(cameraSlot) === -1) {
            return false
        }

        cursorTrackingSessionSlot = cameraSlot
        cursorTrackingSelect = false
        return true
    }

    function cancelCursorTrackingSelection() {
        cursorTrackingSessionSlot = -1
        cursorTrackingSelect = true
    }

    function cancelCursorTrackingSelectionFromBackground() {
        const sessionSlot = cursorTrackingSessionSlot

        if (!cursorTrackingSessionActive || sessionSlot >= cameraTrackingIds.length) {
            cancelCursorTrackingSelection()
            return
        }

        var trackingIds = cameraTrackingIds.slice()
        trackingIds[sessionSlot] = ""
        cameraTrackingIds = trackingIds
        cancelCursorTrackingSelection()
        cursorTrackingSelectionCancelled()
    }

    function recordCursorTarget(cameraSlot, normalizedX, normalizedY) {
        if (!cursorTrackingSessionActive || cameraSlot !== cursorTrackingSessionSlot) {
            return false
        }

        cursorTargetRequest = {
            cameraSlot: cameraSlot,
            normalizedX: normalizedX,
            normalizedY: normalizedY
        }
        cursorTargetRequested(cameraSlot, normalizedX, normalizedY)
        cancelCursorTrackingSelection()
        return true
    }

    function _padRecordTimeSegment(value) {
        return value < 10 ? "0" + value : value.toString()
    }

    function startRecordTimer() {
        recordStartTimeMs = Date.now()
        recordElapsedText = "00:00:00"
    }

    function updateRecordElapsedText() {
        if (!record) {
            recordElapsedText = "00:00:00"
            return
        }

        if (!recordStartTimeMs) {
            startRecordTimer()
            return
        }

        var elapsedSeconds = Math.floor((Date.now() - recordStartTimeMs) / 1000)
        var hours = Math.floor(elapsedSeconds / 3600)
        var minutes = Math.floor((elapsedSeconds % 3600) / 60)
        var seconds = elapsedSeconds % 60

        recordElapsedText = _padRecordTimeSegment(hours) + ":" + _padRecordTimeSegment(minutes) + ":" + _padRecordTimeSegment(seconds)
    }

    function stopRecordTimer() {
        recordStartTimeMs = 0
        recordElapsedText = "00:00:00"
    }

    function toggleHud() {
        hud = !hud
    }

    function toggleToolbar() {
        toolbar = !toolbar
    }

    function toggleAiOverlay() {
        aiOverlay = !aiOverlay
    }

    function toggleLockControls() {
        lockControls = !lockControls
    }

    function toggleSynclairOverlay() {
        synclairOverlay = !synclairOverlay
    }

    function setActiveCameraTrackingId(trackingId) {
        if (!hasActiveCamera) {
            return
        }

        var trackingIds = cameraTrackingIds.slice()
        trackingIds[cameraSelected] = trackingId
        cameraTrackingIds = trackingIds
    }

    function toggleCrosshair() {
        if (cameraSelected < 0 || cameraSelected >= cameraOverlays.length) {
            return
        }

        var overlays = cameraOverlays.slice()
        var selectedOverlays = overlays[cameraSelected]
        overlays[cameraSelected] = {
            grid: selectedOverlays.grid,
            crosshair: !selectedOverlays.crosshair
        }
        cameraOverlays = overlays
    }

    function toggleGrid() {
        if (cameraSelected < 0 || cameraSelected >= cameraOverlays.length) {
            return
        }

        var overlays = cameraOverlays.slice()
        var selectedOverlays = overlays[cameraSelected]
        overlays[cameraSelected] = {
            grid: !selectedOverlays.grid,
            crosshair: selectedOverlays.crosshair
        }
        cameraOverlays = overlays
    }

    function setCamera(cameraId) {
        if (cursorTrackingSessionActive) {
            return
        }

        if (!cameraSelectionEnabled) {
            clearCamera()
            return
        }

        if(cameraId === cameraSelected) {
            clearCamera()
        } else {
            cameraSelected = cameraId
        }
    }

    function clearCamera() {
        cancelCursorTrackingSelection()
        cameraSelected = -1
    }

    function nextCamera() {
        if (cursorTrackingSessionActive) {
            return
        }

        if (!cameraSelectionEnabled) {
            clearCamera()
            return
        }

        cameraSelected = cameraSelected + 1

        if(cameraSelected > 5) {
            cameraSelected = 0
        }
    }

    function toggleRecord() {
        if(record) {
            stopRecording()
        } else {
            startRecording()
        }
    }

    function startRecording() {
        record = true
    }

    function stopRecording() {
        record = false
        stopRecordTimer()
    }

    function takePhoto() {
        var now = Date.now()

        if (now - lastPhotoRequestTimeMs < photoCooldownMs) {
            return
        }

        lastPhotoRequestTimeMs = now
        takePhotoRequested()
    }

//---------------------------------
// Overlay
//---------------------------------
    property bool synclairOverlay: false
    property bool cursorTrackingSelect: true
    property int cursorTrackingSessionSlot: -1
    property var cursorTargetRequest: ({ cameraSlot: -1, normalizedX: 0, normalizedY: 0 })
    readonly property bool shortcutsEnabled: synclairOverlay
    property bool hud: true
    property bool toolbar: true
    property bool lockControls: false
    property string layout: "single"
    property int  cameraSelected: -1
    property bool record: false
    property real recordStartTimeMs: 0
    property string recordElapsedText: "00:00:00"
    property int photoCooldownMs: 500
    property real lastPhotoRequestTimeMs: 0
    property bool aiOverlay: false
    property var cameraTrackingIds: ["", "", "", "", "", ""]
    property var cameraOverlays: [
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false }
    ]
    readonly property bool hasActiveCamera: cameraSelected >= 0 && cameraSelected < cameraTrackingIds.length
    readonly property string activeCameraTrackingId: hasActiveCamera ? cameraTrackingIds[cameraSelected] : ""
    readonly property bool cursorTrackingSessionActive: !cursorTrackingSelect && cursorTrackingSessionSlot >= 0
    readonly property bool grid: cameraSelected >= 0
        && cameraSelected < cameraOverlays.length
        && cameraOverlays[cameraSelected].grid
    readonly property bool crosshair: cameraSelected >= 0
        && cameraSelected < cameraOverlays.length
        && cameraOverlays[cameraSelected].crosshair

    onDigiviewActiveChanged: {
        if (digiviewActive) {
            return
        }

        stopRecording()
        cancelCursorTrackingSelection()
    }

    onCameraSelectedChanged: {
        if (cursorTrackingSessionActive && cameraSelected !== cursorTrackingSessionSlot) {
            cancelCursorTrackingSelection()
        }
    }

    onCameraTrackingIdsChanged: {
        if (cursorTrackingSessionActive && cursorTrackingSessionSlot >= cameraTrackingIds.length) {
            cancelCursorTrackingSelection()
        }
    }
}
