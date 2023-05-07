import QtQuick 2.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Item {
    anchors.fill: parent

    property var screenX
    property var screenY

    property var screenXrateInitCoocked
    property var screenYrateInitCoocked

    property var _gimbalControllerSettings: QGroundControl.settingsManager.gimbalControllerSettings
    property var _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    
    property bool shouldProcessClicks:   QGroundControl.videoManager.fullScreen || flyViewVideoWidgetLayer._gimbalControllerSettings.EnableOnScreenControl.value ||
                                         _activeVehicle

    // Functions for on screen gimbal control
    function clickControl() {
        if (!shouldProcessClicks) {
            return
        }
        if (!_gimbalControllerSettings.ControlType.rawValue == 0) {
            return
        }
        clickAndPoint(x, y)
    }

    // Sends a +-(0-1) xy value to vehicle.gimbalOnScreenControl
    function clickAndPoint() {
        var xCoocked =  ( (screenX / parent.width)  * 2) - 1
        var yCoocked = -( (screenY / parent.height) * 2) + 1
        // console.log("X global: " + x + " Y global: " + y)
        // console.log("X coocked: " + xCoocked + " Y coocked: " + yCoocked)
        _activeVehicle.gimbalOnScreenControl(xCoocked, yCoocked, true, false, false)
    }

    function pressControl() {
        if (!shouldProcessClicks) {
            return
        }
        if (!_gimbalControllerSettings.ControlType.rawValue == 1) {
            return
        }
        sendRateTimer.start()
        screenXrateInitCoocked =  ( ( screenX / parent.width)  * 2) - 1
        screenYrateInitCoocked = -( ( screenY / parent.height) * 2) + 1
    }

    function releaseControl() {
        if (!shouldProcessClicks) {
            return
        }
        if (!_gimbalControllerSettings.ControlType.rawValue == 1) {
            return
        }
        sendRateTimer.stop()
        screenXrateInitCoocked = null
        screenYrateInitCoocked = null 
        // _activeVehicle.gimbalOnScreenControl(0, 0, false, true, true)
    }

    Timer {
        id:             sendRateTimer
        interval:       100
        repeat:         true
        onTriggered: {
            if (_activeVehicle) {
                var xCoocked =  ( ( screenX / parent.width)  * 2) - 1
                var yCoocked = -( ( screenY / parent.height) * 2) + 1
                xCoocked -= screenXrateInitCoocked
                yCoocked -= screenYrateInitCoocked
                _activeVehicle.gimbalOnScreenControl(xCoocked, yCoocked, false, true, true)
            }
        }
    }

    // Pitch/Roll indicators
    Rectangle {
        id: pitchRollRectangle
        width: gimbalPitchLabel.width + gimbalPanLabel.width + ScreenTools.defaultFontPixelWidth * 5
        height: gimbalPitchLabel.height + ScreenTools.defaultFontPixelWidth * 2
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: qgcPal.window
        opacity: 0.8

        QGCLabel {
            id: gimbalPitchLabel
            text: _activeVehicle && _activeVehicle.gimbalData ? "Tilt: " + _activeVehicle.gimbalPitch.toFixed(2) : ""
            visible: _activeVehicle && _activeVehicle.gimbalData
            anchors.top: parent.top
            anchors.left: parent.horizontalCenter
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }

        QGCLabel {
            id: gimbalPanLabel
            text: _activeVehicle && _activeVehicle.gimbalData ? "Pan: " + _activeVehicle.gimbalYaw.toFixed(2) : ""
            visible: _activeVehicle && _activeVehicle.gimbalData
            anchors.top: parent.top
            anchors.right: parent.horizontalCenter
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
    }
}