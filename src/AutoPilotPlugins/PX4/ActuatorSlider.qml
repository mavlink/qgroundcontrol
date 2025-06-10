import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
    property var channel
    property alias value:             channelSlider.value
    property int motorIndex:          -1  // Motor index for telemetry, -1 for non-motors
    property var escStatus:           null // ESC status object

    // If the default value is NaN, we add a small range
    // below, which snaps into place
    property var snap:                isNaN(channel.defaultValue)
    property var span:                channel.max - channel.min
    property var snapRange:           span * 0.15
    property var defaultVal:          snap ? channel.min - snapRange : channel.defaultValue
    property var blockUpdates:        true // avoid slider changes on startup

    id:                               root
    width:                            60  // Fixed width for proper spacing
    height:                           ScreenTools.defaultFontPixelHeight * 6

    Layout.alignment:                 Qt.AlignTop

    // ESC telemetry helper functions
    function _isMotorHealthy() {
        if (!escStatus || motorIndex < 0 || !escStatus.telemetryAvailable) return false
        var infoBitmask = escStatus.info.rawValue
        var isOnline = (infoBitmask & (1 << motorIndex)) !== 0
        if (!isOnline) return false
        var failureFlags = _getMotorFailureFlags()
        return failureFlags === 0
    }

    function _getMotorFailureFlags() {
        if (!escStatus || motorIndex < 0) return 0
        switch (motorIndex) {
        case 0: return escStatus.failureFlagsFirst.rawValue
        case 1: return escStatus.failureFlagsSecond.rawValue
        case 2: return escStatus.failureFlagsThird.rawValue
        case 3: return escStatus.failureFlagsFourth.rawValue
        case 4: return escStatus.failureFlagsFifth.rawValue
        case 5: return escStatus.failureFlagsSixth.rawValue
        case 6: return escStatus.failureFlagsSeventh.rawValue
        case 7: return escStatus.failureFlagsEighth.rawValue
        default: return 0
        }
    }

    function _getMotorRPM() {
        if (!escStatus || motorIndex < 0) return 0
        switch (motorIndex) {
        case 0: return escStatus.rpmFirst.rawValue
        case 1: return escStatus.rpmSecond.rawValue
        case 2: return escStatus.rpmThird.rawValue
        case 3: return escStatus.rpmFourth.rawValue
        case 4: return escStatus.rpmFifth.rawValue
        case 5: return escStatus.rpmSixth.rawValue
        case 6: return escStatus.rpmSeventh.rawValue
        case 7: return escStatus.rpmEighth.rawValue
        default: return 0
        }
    }

    readonly property int _sliderHeight: 5

    function stopTimer() {
        sendTimer.stop();
    }

    function stop() {
        channelSlider.value = defaultVal;
        stopTimer();
    }

    signal actuatorValueChanged(real value, real sliderValue)

    QGCSlider {
        id:                         channelSlider
        orientation:                Qt.Vertical
        from:               snap ? channel.min - snapRange : channel.min
        to:               channel.max
        stepSize:                   (channel.max-channel.min)/100
        value:                      defaultVal
        live:   true
        anchors.horizontalCenter:   parent.horizontalCenter
        height:                     ScreenTools.defaultFontPixelHeight * _sliderHeight
        indicatorBarVisible:        sendTimer.running

        onValueChanged: {
            if (blockUpdates)
                return;
            if (snap) {
                if (value < channel.min) {
                    if (value < channel.min - snapRange/2) {
                        value = channel.min - snapRange;
                    } else {
                        value = channel.min;
                    }
                }
            }
            sendTimer.start()
        }

        Timer {
            id:               sendTimer
            interval:         50
            triggeredOnStart: true
            repeat:           true
            running:          false
            onTriggered:      {
                var sendValue = channelSlider.value;
                if (sendValue < channel.min - snapRange/2) {
                    sendValue = channel.defaultValue;
                }
                root.actuatorValueChanged(sendValue, channelSlider.value)
            }
        }

        Component.onCompleted: {
            blockUpdates = false;
        }
    }

    // Horizontal motor label at bottom
    QGCLabel {
        text: motorIndex >= 0 ? (motorIndex + 1).toString() : qsTr("all")
        font.pointSize: ScreenTools.smallFontPointSize
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        visible: channel.isMotor || motorIndex === -1
    }

    // RPM value positioned to middle-right of slider
    QGCLabel {
        visible: motorIndex >= 0 && channel.isMotor && escStatus && escStatus.telemetryAvailable
        text: _isMotorHealthy() ? _getMotorRPM().toString() : qsTr("ERR")
        font.pointSize: ScreenTools.smallFontPointSize * 0.8
        color: _isMotorHealthy() ? qgcPal.text : qgcPal.colorRed
        x: channelSlider.x + channelSlider.width + 5
        y: channelSlider.y + channelSlider.height / 2 - height / 2
    }

    // Fallback to original vertical label for non-motors
    QGCLabel {
        visible: !channel.isMotor && motorIndex !== -1
        anchors.horizontalCenter: parent.horizontalCenter
        text: channel.label
        width: contentHeight
        height: contentWidth
        transform: [
            Rotation { origin.x: 0; origin.y: 0; angle: -90 },
            Translate { y: height + 5 }
        ]
    }
} // Item
