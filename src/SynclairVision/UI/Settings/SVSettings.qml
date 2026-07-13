pragma Singleton
import QtQuick

QtObject {
    id: root

//---------------------------------
// General
//---------------------------------

    //Video
        property int videoResolutionWidth: 1280
        property int videoResolutionHeight: 720
        property int videoFps: 30
        property real videoTargetBrightness: 1
        property bool recordHighlight: true
        property bool recordInformationBox: true

    //Network
        property string networkIPAdress: "127.0.0.1"
        property bool networkConnected: true

    //Calibration
        property string calibrationCommand: "test"
        property bool calibrationActive: false

//---------------------------------
// Controls
//---------------------------------
    //Control Panel
        property bool controlPanel: true
        property string controlPanelPosition: "Bottom-center"
        property bool controlPanelInteraction: true //Press or click
        property bool controlPanelPassiveOpacity: false
        property real controlPanelPassiveOpacityValue: 1

    //Joystick
        property string joystickType: "standard"
        property int joystickSize: 40
        property int joystickSensitivity: 10
        property real joystickDeadzone: 0
        property bool joystickInvertHorizontal: false
        property bool joystickInvertVertical: false
        property real joystickRatio: 0.5
        property real joystickKnobSize: 0.3

    //Zoom
        property int zoomSize: 25
        property int zoomSensitivity: 10

    //Shortcuts
        property int shortcutPitchUp: Qt.Key_Up
        property int shortcutPitchDown: Qt.Key_Up
        property int shortcutJawLeft: Qt.Key_Up
        property int shortcutJawRight: Qt.Key_Up
        //property int shortcutRollLeft: Qt.Key_Up
        //property int shortcutRollRight: Qt.Key_Up
        property int shortcutZoomIn: Qt.Key_Up
        property int shortcutZoomOut: Qt.Key_Up
        property int shortcutSlowMovement: Qt.Key_Up
        property int shortcutSynclair: Qt.Key_Up
        property int shortcutHUD: Qt.Key_Up
        property int shortcutToolbar: Qt.Key_Up
        property int shortcutLayout: Qt.Key_Up
        property int shortcutPhoto: Qt.Key_Up
        property int shortcutRecord: Qt.Key_Up
        property int shortcutCamera1: Qt.Key_Up
        property int shortcutCamera2: Qt.Key_Up
        property int shortcutCamera3: Qt.Key_Up
        property int shortcutCamera4: Qt.Key_Up
        property int shortcutCamera5: Qt.Key_Up
        property int shortcutNextCamera: Qt.Key_Up
        property int shortcutDeselectCamera: Qt.Key_Up

        onControlPanelPositionChanged: console.log("panel position ->", controlPanelPosition)



//---------------------------------
// Developer
//---------------------------------
    //AI
        property string aiDetectionOverlay: "right"
        property string aiSortingMode: "test"
        property int aiCropConfidenceTreshold
        property int aiScanConfidenceTreshold
        property int aiCreationScoreScale
        property int aiBonusDetectionScale
        property int aiBonusRedetectionScale
        property int aiMissedDetectionPenaltyScale
        property int aiMissedRedetectionPenaltyScale
        property int aiCropBoxOverlay
        property int aiVarBoxOverlap

    //Camera
        property int cameraMinimalExposure
        property int cameraMaximalExposure
        property int cameraMinimalGain
        property int cameraMaximalGain
}