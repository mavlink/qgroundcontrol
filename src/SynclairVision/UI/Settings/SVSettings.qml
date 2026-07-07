pragma Singleton
import QtQuick

QtObject {
    id: root

//---------------------------------
// Overlay
//---------------------------------

    property bool synclairOverlay: true
    property bool hud: true
    property bool toolbar: true
    property bool lockControls: false
    property string layout: "single"
    property bool record: false

//---------------------------------
// General
//---------------------------------

    //Video
        property int videoResolution: 123
        property real targetBrightness: 1

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
        property int joystickSize: 200
        property int joystickSensitivity: 10
        property real joystickDeadzone: 0
        property bool joystickInvertHorizontal: false
        property bool joystickInvertVertical: false

    //Zoom
        property int zoomSize: 100
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