pragma Singleton
import QtQuick

QtObject {
    id: root

    signal takePhotoRequested()

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

    function toggleLockControls() {
        lockControls = !lockControls
    }

    function toggleSynclairOverlay() {
        synclairOverlay = !synclairOverlay
    }

    function setCamera(cameraId) {
        if(cameraId === cameraSelected) {
            clearCamera()
        } else {
            cameraSelected = cameraId
        }
    }

    function clearCamera() {
        cameraSelected = -1
    }

    function nextCamera() {
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
    property bool synclairOverlay: true
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
}
