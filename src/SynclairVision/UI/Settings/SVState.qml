pragma Singleton
import QtQuick

import QGroundControl

QtObject {
    id: root


    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool digiviewActive: !!(digiview && digiview.connected)
    property bool userInitiatedDisconnect: false
    // True once the current connection attempt has actually reached the "connected" (decoding) state.
    // Reset every time a new connection attempt starts, so we can tell "was connected, then dropped"
    // apart from "was still connecting, then cancelled/failed".
    property bool _reachedConnectedState: false
    // Guards against reporting "Connection Lost" twice for the same drop — decoding usually stops
    // (uiInteractionEnabled -> false) well before the socket itself notices (digiviewActive -> false),
    // since that can rely on an OS-level TCP timeout that may take a long time or never fire cleanly.
    property bool _connectionLostReported: false
    readonly property string synclairOverlayVideoUri: {
        const profile = SVSettings.selectedNetworkProfile()
        const videoPortText = profile ? SVSettings.networkProfileText(profile.videoPort) : ""

        if (!profile
                || SVSettings.networkProfileText(profile.host) === ""
                || !/^\d+$/.test(videoPortText)
                || SVSettings.networkProfileText(profile.streamName) === "") {
            return ""
        }

        const videoPort = SVSettings.networkProfilePort(profile.videoPort, -1)
        if (videoPort <= 0 || videoPort > 65535) {
            return ""
        }

        return SVSettings.networkProfileRtspUrl(profile)
    }
    readonly property bool synclairOverlayVideoActive: synclairOverlay
        && digiviewActive
        && synclairOverlayVideoUri !== ""
    readonly property bool uiInteractionEnabled: digiviewActive && QGroundControl.videoManager.decoding
    readonly property bool cameraSelectionEnabled: uiInteractionEnabled

    signal cursorTargetRequested(int cameraSlot, real normalizedX, real normalizedY)
    signal cursorTrackingSelectionCancelled()
    signal pointTrackingSelectionRequested(string trackingId)

    function beginPointTrackingSelection(trackingId, cameraSlot, visibleCameraSlots) {
        if (!cameraSelectionEnabled || cameraSlot !== cameraSelected
                || cameraSlot < 0 || cameraSlot >= cameraTrackingIds.length
                || !visibleCameraSlots || visibleCameraSlots.indexOf(cameraSlot) === -1
                || (trackingId !== 'cursorTrack' && trackingId !== 'singleTarget')) {
            return false
        }

        cursorTrackingSessionSlot = cameraSlot
        pointTrackingSelectionMode = trackingId
        cursorTrackingSelect = false
        return true
    }

    function _endPointTrackingSelection() {
        cursorTrackingSessionSlot = -1
        pointTrackingSelectionMode = ''
        cursorTrackingSelect = true
    }

    function cancelCursorTrackingSelection() {
        if (cursorTrackingSessionActive
                && cursorTrackingSessionSlot >= 0
                && cursorTrackingSessionSlot < cameraTrackingIds.length
                && cameraTrackingIds[cursorTrackingSessionSlot] === pointTrackingSelectionMode) {
            setCameraTrackingId(cursorTrackingSessionSlot, '')
        }

        _endPointTrackingSelection()
    }

    function cancelCursorTrackingSelectionFromBackground() {
        const sessionSlot = cursorTrackingSessionSlot
        if (!cursorTrackingSessionActive || sessionSlot < 0) {
            cancelCursorTrackingSelection()
            return
        }

        cancelCursorTrackingSelection()
        cursorTrackingSelectionCancelled()
    }

    function recordCursorTarget(cameraSlot, normalizedX, normalizedY) {
        if (!cursorTrackingSessionActive || cameraSlot !== cursorTrackingSessionSlot
                || !Number.isFinite(normalizedX) || !Number.isFinite(normalizedY)) {
            return false
        }

        var submitted = false
        if (pointTrackingSelectionMode === 'singleTarget') {
            submitted = digiview && digiview.setSingleTargetTrackingTarget(
                cameraSlot, normalizedX, normalizedY)
        } else if (pointTrackingSelectionMode === 'cursorTrack') {
            submitted = digiview && digiview.setCameraCursorTarget(
                cameraSlot, normalizedX, normalizedY)
        }

        if (!submitted) {
            cancelCursorTrackingSelection()
            return false
        }

        cursorTargetRequest = {
            cameraSlot: cameraSlot,
            normalizedX: normalizedX,
            normalizedY: normalizedY
        }
        setActiveCameraTrackingId(pointTrackingSelectionMode, true)
        cursorTargetRequested(cameraSlot, normalizedX, normalizedY)
        _endPointTrackingSelection()
        return true
    }

    function submitImmediatePointTracking(trackingId, cameraSlot, normalizedX, normalizedY) {
        if (!cameraSelectionEnabled || lockControls
                || (trackingId !== 'singleTarget' && trackingId !== 'cursorTrack')
                || cameraSlot < 0 || cameraSlot >= cameraTrackingIds.length
                || !Number.isFinite(normalizedX) || !Number.isFinite(normalizedY)
                || normalizedX < -1.0 || normalizedX > 1.0
                || normalizedY < -1.0 || normalizedY > 1.0 || !digiview) {
            return false
        }

        const submitted = trackingId === 'singleTarget'
            ? digiview.setSingleTargetTrackingTarget(cameraSlot, normalizedX, normalizedY)
            : digiview.setCameraCursorTarget(cameraSlot, normalizedX, normalizedY)
        if (!submitted) {
            return false
        }

        cursorTargetRequest = { cameraSlot: cameraSlot, normalizedX: normalizedX, normalizedY: normalizedY }
        setCameraTrackingId(cameraSlot, trackingId, true)
        cursorTargetRequested(cameraSlot, normalizedX, normalizedY)
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

    function aiDetectionOverlayModeForPosition(positionId) {
        switch (positionId) {
        case 'Single':
            return DigiviewProtocol.DetectionOverlaySingleTopRight
        case 'ColumnRight':
            return DigiviewProtocol.DetectionOverlayColumnRight
        case 'ColumnLeft':
            return DigiviewProtocol.DetectionOverlayColumnLeft
        case 'RowTop':
            return DigiviewProtocol.DetectionOverlayRowTop
        case 'RowBottom':
            return DigiviewProtocol.DetectionOverlayRowBottom
        default:
            return DigiviewProtocol.DetectionOverlayNone
        }
    }

    function aiDetectionOverlayPositionForMode(mode) {
        switch (mode) {
        case DigiviewProtocol.DetectionOverlaySingleTopRight:
            return 'Single'
        case DigiviewProtocol.DetectionOverlayColumnRight:
            return 'ColumnRight'
        case DigiviewProtocol.DetectionOverlayColumnLeft:
            return 'ColumnLeft'
        case DigiviewProtocol.DetectionOverlayRowTop:
            return 'RowTop'
        case DigiviewProtocol.DetectionOverlayRowBottom:
            return 'RowBottom'
        default:
            return ''
        }
    }

    function sendAiDetectionOverlayMode(mode) {
        if (!hasCurrentVideoOutputState) {
            return
        }

        digiview.setDetectionOverlayMode(mode)
    }

    function setAiDetectionOverlayPosition(positionId) {
        const mode = aiDetectionOverlayModeForPosition(positionId)

        if (mode !== DigiviewProtocol.DetectionOverlayNone) {
            sendAiDetectionOverlayMode(mode)
        }
    }

    function toggleAiOverlay() {
        if (aiOverlay) {
            sendAiDetectionOverlayMode(DigiviewProtocol.DetectionOverlayNone)
            return
        }

        setAiDetectionOverlayPosition(effectiveAiDetectionOverlayPosition)
    }

    function toggleLockControls() {
        lockControls = !lockControls
        return
    }

    function toggleSynclairOverlay() {
        synclairOverlay = !synclairOverlay
    }

    function _reportConnectionLost(profileName, profileHost) {
        if (_connectionLostReported) {
            return
        }
        _connectionLostReported = true

        SVNotificationManager.add(
            "Connection Lost",
            "Unexpectedly lost connection to " + "<i>" + profileName + "</i>" + ".",
            "error",
            "network_disconnected"
        )

        stopRecording()
        cancelCursorTrackingSelection()
    }

    function changeEuler(direction, smallMovement, strength) {
        if (!digiview || !hasActiveCamera || lockControls) {
            return
        }

        let yaw = 0
        let pitch = 0

        if (smallMovement) {
            strength *= 0.333
        }

        switch (direction) {
        case 0:
            yaw = strength
            break
        case 1:
            pitch = -strength
            break
        case 2:
            yaw = -strength
            break
        case 3:
            pitch = strength
            break
        default:
            return
        }

        if (SVSettings.invertJoystickX) {
            yaw = -yaw
        }

        if (SVSettings.invertJoystickY) {
            pitch = -pitch
        }

        digiview.changeEuler(cameraSelected, yaw, pitch)
    }

    function changeZoom(zoom) {
        if (!digiview || !hasActiveCamera || lockControls) {
            return
        }

        digiview.changeZoom(cameraSelected, zoom)
    }

    function setCameraTrackingId(cameraSlot, trackingId, awaitingConfirmation) {
        if (cameraSlot < 0 || cameraSlot >= cameraTrackingIds.length) {
            return
        }

        var trackingIds = cameraTrackingIds.slice()
        trackingIds[cameraSlot] = trackingId
        cameraTrackingIds = trackingIds

        var pending = cameraTrackingAwaitingConfirmation.slice()
        pending[cameraSlot] = trackingId !== '' && awaitingConfirmation === true
        cameraTrackingAwaitingConfirmation = pending
    }

    function setActiveCameraTrackingId(trackingId, awaitingConfirmation) {
        if (!hasActiveCamera) {
            return
        }

        setCameraTrackingId(cameraSelected, trackingId, awaitingConfirmation)
    }

    property int layoutIndex: DigiviewProtocol.LayoutSingleCamera
    readonly property int layoutCount: DigiviewProtocol.LayoutMaximum + 1

    function nextLayout() {
        if (!digiview || !digiviewActive) {
            return
        }

        const nextLayoutIndex = (layoutIndex + 1) % layoutCount
        if (digiview.setVideoOutputLayout(nextLayoutIndex)) {
            layoutIndex = nextLayoutIndex
        }
    }

    function activateSttTracking() {
        if (!digiview || !hasActiveCamera || lockControls) {
            return
        }

        pointTrackingSelectionRequested('singleTarget')
    }

    function activateCursorTracking() {
        if (!hasActiveCamera || lockControls) {
            return
        }

        pointTrackingSelectionRequested('cursorTrack')
    }

    function activateManualTracking() {
        if (!digiview || !hasActiveCamera || lockControls) {
            return false
        }

        const latitudeText = SVSettings.trackingLatitude.trim()
        const longitudeText = SVSettings.trackingLongitude.trim()
        const altitudeText = SVSettings.trackingAltitude.trim()
        if (latitudeText === '' || longitudeText === '' || altitudeText === '') {
            return false
        }

        const latitude = Number(latitudeText)
        const longitude = Number(longitudeText)
        const altitude = Number(altitudeText)
        if (!Number.isFinite(latitude) || !Number.isFinite(longitude) || !Number.isFinite(altitude)
                || !digiview.setCameraManualTarget(cameraSelected, latitude, longitude, altitude)) {
            return false
        }

        setActiveCameraTrackingId('manual', true)
        return true
    }

    function deselectTracking() {
        if (!hasActiveCamera) {
            return
        }

        stopActiveTracking()
    }

    function stopActiveTracking() {
        const cameraSlot = cameraSelected
        const trackingId = cameraSlot >= 0 && cameraSlot < cameraTrackingIds.length
            ? cameraTrackingIds[cameraSlot]
            : activeCameraTrackingId

        if (trackingId !== '') {
            if (!digiview || !digiview.clearCurrentTarget(cameraSlot)) {
                return false
            }
            setActiveCameraTrackingId('')
        }

        cancelCursorTrackingSelection()
        return true
    }

    function synchronizeCameraTrackingStates() {
        if (!digiview || !digiview.cameraStates) {
            return
        }

        const cameraStates = digiview.cameraStates
        const cameraCount = Math.min(cameraTrackingIds.length, cameraStates.length)
        for (let cameraSlot = 0; cameraSlot < cameraCount; ++cameraSlot) {
            const state = cameraStates[cameraSlot]
            if (!state || !state.hasTargetState) {
                continue
            }

            if (state.hasActiveTarget) {
                if (cameraTrackingAwaitingConfirmation[cameraSlot]) {
                    setCameraTrackingId(cameraSlot, cameraTrackingIds[cameraSlot], false)
                }
            } else if (!cameraTrackingAwaitingConfirmation[cameraSlot]
                       && cameraTrackingIds[cameraSlot] !== '') {
                setCameraTrackingId(cameraSlot, '')
            }
        }
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

    function previousCamera() {
        if (cursorTrackingSessionActive) {
            return
        }

        if (!cameraSelectionEnabled) {
            clearCamera()
            return
        }

        cameraSelected = cameraSelected - 1

        if (cameraSelected < 0) {
            cameraSelected = 5
        }
    }

    function toggleRecord() {
        if(record) {
            stopRecording()
        } else {
            startRecording()
        }
    }

    function recordingFileName() {
        const now = new Date()

        const year = now.getFullYear()
        const month = String(now.getMonth() + 1).padStart(2, '0')
        const day = String(now.getDate()).padStart(2, '0')
        const hour = String(now.getHours()).padStart(2, '0')
        const minute = String(now.getMinutes()).padStart(2, '0')
        const second = String(now.getSeconds()).padStart(2, '0')

        return `Synclair_${year}-${month}-${day}_${hour}-${minute}-${second}`
    }

    function startRecording() {
        record = true
        if(SVSettings.recordDestination === "digiview") {
            digiview.startRecording()
            return
        }

        if(SVSettings.recordDestination === "local") {
            QGroundControl.videoManager.startRecording(recordingFileName());
        }
    }

    function stopRecording() {
        if(record === true) {
            if(SVSettings.recordDestination === "digiview") {
                digiview.stopRecording()
                SVNotificationManager.add(
                    "Recording Stopped",
                    "Digiview: Saving recording to Jetson",
                    "info",
                    "recording_stopped"
                ) 
                return
            }
            
            if(SVSettings.recordDestination === 'local') {
                QGroundControl.videoManager.stopRecording();
                SVNotificationManager.add(
                    "Recording Stopped",
                    "QGroundControl: Saving recording to local folder",
                    "info",
                    "recording_stopped"
                ) 
            }
        }
        
        record = false
        stopRecordTimer()
    }


    function takePhoto() {
        var now = Date.now()

        if (now - lastPhotoRequestTimeMs < photoCooldownMs) {
            return
        }

        lastPhotoRequestTimeMs = now
        if (digiview) {
            digiview.takePhoto()
        }
    }

//---------------------------------
// Overlay
//---------------------------------
    property bool synclairOverlay: false
    property bool cursorTrackingSelect: true
    property int cursorTrackingSessionSlot: -1
    property string pointTrackingSelectionMode: ''
    property var cursorTargetRequest: ({ cameraSlot: -1, normalizedX: 0, normalizedY: 0 })
    readonly property bool shortcutsEnabled: synclairOverlay
    property bool hud: true
    property bool toolbar: true
    property bool lockControls: false
    property var shortcutJoystickHeld: [false, false, false, false]
    property bool shortcutZoomInHeld: false
    property bool shortcutZoomOutHeld: false
    property bool shortcutSmallMovementHeld: false
    property int  cameraSelected: -1
    property bool record: false
    property real recordStartTimeMs: 0
    property string recordElapsedText: "00:00:00"
    property int photoCooldownMs: 500
    property real lastPhotoRequestTimeMs: 0
    readonly property bool hasCurrentVideoOutputState: !!digiview
        && digiview.connected
        && digiview.hasVideoOutputParameters
        && digiview.videoOutputStreamName === digiview.streamName
    readonly property bool aiOverlay: hasCurrentVideoOutputState
        && aiDetectionOverlayPositionForMode(digiview.videoOutputDetectionOverlayMode) !== ''
    readonly property string effectiveAiDetectionOverlayPosition: {
        if (hasCurrentVideoOutputState) {
            const remotePosition = aiDetectionOverlayPositionForMode(digiview.videoOutputDetectionOverlayMode)
            if (remotePosition !== '') {
                return remotePosition
            }
        }

        return aiDetectionOverlayModeForPosition(SVSettings.aiDetectionOverlayPosition)
                !== DigiviewProtocol.DetectionOverlayNone
            ? SVSettings.aiDetectionOverlayPosition
            : 'Single'
    }
    property var cameraTrackingIds: ["", "", "", "", "", ""]
    property var cameraTrackingAwaitingConfirmation: [false, false, false, false, false, false]
    property var cameraOverlays: [
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false },
        { grid: false, crosshair: false }
    ]
    readonly property bool hasActiveCamera: cameraSelected >= 0 && cameraSelected < cameraTrackingIds.length
    
    readonly property var activeCameraState: (digiview && digiview.cameraStates && cameraSelected >= 0 && cameraSelected < digiview.cameraStates.length)
                                             ? digiview.cameraStates[cameraSelected]
                                             : null

    readonly property bool isCurrentCamTracking: activeCameraState
        ? activeCameraState.sttStatus === DigiviewProtocol.SttStatusRunning
        : false
    
    
    readonly property int activeCameraTrackId: activeCameraState ? activeCameraState.trackId : 0

    // Keep the local mode after a successful selection. Remote state only supplies
    // a menu mode where the camera-state fields identify one unambiguously.
    readonly property string activeCameraTrackingId: {
        if (hasActiveCamera && cameraTrackingIds[cameraSelected]) {
            return cameraTrackingIds[cameraSelected]
        }

        if (activeCameraState && activeCameraState.targetingMode === DigiviewProtocol.TargetingDetection) {
            return 'detection'
        }

        if (activeCameraState && activeCameraState.sttStatus === DigiviewProtocol.SttStatusRunning) {
            return 'singleTarget'
        }

        return ""
    }
    readonly property bool cursorTrackingSessionActive: !cursorTrackingSelect && cursorTrackingSessionSlot >= 0
    readonly property bool grid: cameraSelected >= 0
        && cameraSelected < cameraOverlays.length
        && cameraOverlays[cameraSelected].grid
    readonly property bool crosshair: cameraSelected >= 0
        && cameraSelected < cameraOverlays.length
        && cameraOverlays[cameraSelected].crosshair

    property Connections _cameraStatesConnection: Connections {
        target: digiview

        function onCameraStatesChanged() {
            root.synchronizeCameraTrackingStates()
        }

        function onVideoOutputLayoutModeChanged() {
            const layoutMode = digiview.videoOutputLayoutMode
            if (layoutMode >= DigiviewProtocol.LayoutSingleCamera
                    && layoutMode <= DigiviewProtocol.LayoutMaximum) {
                root.layoutIndex = layoutMode
            }
        }

        function onCommandRejected(reason) {
            SVNotificationManager.add(
                qsTr("DigiView Command Rejected"),
                reason,
                "warning",
                "network_error"
            )
        }
    }

    onDigiviewActiveChanged: {
        var profile = SVSettings.selectedNetworkProfile()
        var profileName = profile ? profile.name : "Stream"
        var profileHost = profile ? profile.host : "Unknown Host"

        if (digiviewActive) {
            // State 1: Connecting (Socket open, but waiting for video)
            _reachedConnectedState = false
            _connectionLostReported = false

            SVNotificationManager.add(
                "Connecting...",
                "Attempting a connection to " + "<i>" + profileName + "</i>" + ".",
                "info",
                "network_connecting"
            )
        } else {
            // State 3: Disconnected
            cameraTrackingIds = ["", "", "", "", "", ""]
            cameraTrackingAwaitingConfirmation = [false, false, false, false, false, false]

            if (userInitiatedDisconnect) {
                if (_reachedConnectedState) {
                    SVNotificationManager.add(
                        "Disconnected",
                        "Manually disconnected from " + "<i>" + profileName + "</i>" + ".",
                        "warning",
                        "network_disconnected"
                    )
                } else {
                    SVNotificationManager.add(
                        "Connecting Canceled",
                        "Canceled connection attempt to " + "<i>" + profileName + "</i>" + ".",
                        "info",
                        "network_disconnected"
                    )
                }

                userInitiatedDisconnect = false
                _reachedConnectedState = false
                _connectionLostReported = false
                stopRecording()
                cancelCursorTrackingSelection()
            } else {
                // Socket finally noticed the drop. If decoding already reported it (the common case
                // when wifi just vanishes), this is a no-op — _reportConnectionLost only fires once.
                _reportConnectionLost(profileName, profileHost)
                _reachedConnectedState = false
            }
        }
    }

    

    onUiInteractionEnabledChanged: {
        // State 2: Connected (Stream is actively decoding)
        // We ensure devBypassDisconnectedUiDisable isn't the reason it turned true
        if (uiInteractionEnabled && digiviewActive) {
            _reachedConnectedState = true

            var profile = SVSettings.selectedNetworkProfile()
            var profileName = profile ? profile.name : "Stream"
            var profileHost = profile ? profile.host : "Unknown Host"

            SVNotificationManager.add(
                "Connected",
                "Stream successfully connected to " + "<i>" + profileName + "</i>" + ".",
                "success",
                "network_connected"
            )
        } else if (!uiInteractionEnabled && digiviewActive && _reachedConnectedState && !userInitiatedDisconnect) {
            // Decoding stopped while the socket still thinks it's connected — e.g. wifi was cut.
            // This is usually the first (and sometimes only) signal we get that the stream died,
            // since the socket-level "connected" flag can lag far behind (or never flip on its own).
            var lostProfile = SVSettings.selectedNetworkProfile()
            var lostProfileName = lostProfile ? lostProfile.name : "Stream"
            var lostProfileHost = lostProfile ? lostProfile.host : "Unknown Host"

            _reportConnectionLost(lostProfileName, lostProfileHost)
        }
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
