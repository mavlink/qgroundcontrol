import QtQuick 2.12
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.3

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Column {
    property var channel
    property alias value:             channelSlider.value

    // If the default value is NaN, we add a small range
    // below, which snaps into place
    property var isBidirectionalMotor:  channel.isBidirectional
    property var isStandardMotor:           channel.isMotor && !channel.isBidirectional

    property var snap:                isNaN(channel.defaultValue)
    property var span:                channel.max - channel.min
    property var snapRange:           span * 0.15
    // property var defaultVal:          snap ? channel.min - snapRange : channel.defaultValue
    property var defaultVal:          channel.isBidirectional ? (channel.max + channel.min)/2 : channel.isStandardMotor ? channel.min - snapRange : channel.defaultVal

    property var blockUpdates:        true // avoid slider changes on startup

    id:                               root

    Layout.alignment:                 Qt.AlignTop

    readonly property int _sliderHeight: 6

    function stopTimer() {
        sendTimer.stop();
    }

    function stop() {
        channelSlider.value = channel.defaultValue
        stopTimer();
    }

    signal actuatorValueChanged(real value, real sliderValue)

    QGCSlider {
        id:                         channelSlider
        orientation:                Qt.Vertical
        minimumValue:               isStandardMotor ? channel.min - snapRange : channel.min
        // minimumValue:               snap ? channel.min - snapRange : channel.min
        maximumValue:               channel.max
        stepSize:                   (channel.max-channel.min)/100
        value:                      channel.defaultValue
        updateValueWhileDragging:   true
        anchors.horizontalCenter:   parent.horizontalCenter
        height:                     ScreenTools.defaultFontPixelHeight * _sliderHeight
        indicatorBarVisible:        sendTimer.running

        onValueChanged: {
            if (blockUpdates)
                return;

            if(isStandardMotor){
                if (value < channel.min) {
                    if (value < channel.min - snapRange/2) {
                        value = channel.min - snapRange;
                    } else {
                        value = channel.min;
                    }
                }

            } else if(isBidirectionalMotor){

                if (value > channel.defaultValue - snapRange/2 && value < 0.0) {
                    value = 0.0

                } else if (value < channel.defaultValue + snapRange/2 && value > 0.0) {
                    value = 0.0

                // } else if(value < channel.defaultValue - snapRange/2) {
                //     value = channel.defaultValue - snapRange

                // } else if(value > channel.defaultValue + snapRange/2) {
                //     value = channel.defaultValue + snapRange


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

                if(isStandardMotor){

                    if (sendValue < channel.min - snapRange/2) {
                        sendValue = channel.defaultValue;
                    }

                }
                else if(isBidirectionalMotor){

                    if (sendValue > 0.0 - snapRange && sendValue < 0.0) {
                        sendValue = 0.0
                    }
                    else if (sendValue < 0.0 + snapRange && sendValue > 0.0) {
                        sendValue = channel.defaultValue;
                    }
                    else if(sendValue > 0.0 + snapRange){
                        sendValue = sendValue - 0.0 + snapRange
                    }
                    else if(sendValue < 0.0 - snapRange){
                        sendValue = sendValue + 0.0 + snapRange
                    }
                }
                root.actuatorValueChanged(sendValue, channelSlider.value)
            }
        }

        Component.onCompleted: {
            blockUpdates = false;
        }
    }

    QGCLabel {
        id: channelLabel
        anchors.horizontalCenter: parent.horizontalCenter
        text:                     channel.label
        width:                    contentHeight
        height:                   contentWidth
        transform: [
            Rotation { origin.x: 0; origin.y: 0; angle: -90 },
            Translate { y: channelLabel.height + 5 }
            ]
    }
} // Column
