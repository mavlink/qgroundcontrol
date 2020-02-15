/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

SetupPage {
    id:             motorPage
    pageComponent:  pageComponent
    enabled:        true

    readonly property int _barHeight:       10
    readonly property int _barWidth:        5
    readonly property int _sliderHeight:    10

    property int neutralValue: 50;
    property int _lastIndex: 0;
    property bool canDoManualTest: controller.vehicle.flightMode !== 'Motor Detection' && controller.vehicle.armed && motorPage.visible

    APMSubMotorComponentController {
        id:             controller
    }

    function setMotorDirection(num, reversed) {
        var fact = controller.getParameterFact(-1, "MOT_" + num + "_DIRECTION")
        fact.value = reversed ? -1 : 1;
    }

    Component.onCompleted: controller.vehicle.armed = false

    Component {
        id: pageComponent

        Column {
            spacing: 10

            Row {
                id:         motorSliders
                enabled:    canDoManualTest
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                Column {
                    spacing:    ScreenTools.defaultFontPixelWidth * 2

                    Row {
                        id: sliderRow
                        spacing:    ScreenTools.defaultFontPixelWidth * 4

                        Repeater {
                            id:         sliderRepeater
                            model:      controller.vehicle.motorCount == -1 ? 8 : controller.vehicle.motorCount

                            Column {
                                property alias motorSlider: slider
                                spacing:    ScreenTools.defaultFontPixelWidth

                                QGCLabel {
                                    anchors.horizontalCenter:   parent.horizontalCenter
                                    text:                       index + 1
                                }

                                QGCSlider {
                                    id:                         slider
                                    height:                     ScreenTools.defaultFontPixelHeight * _sliderHeight
                                    orientation:                Qt.Vertical
                                    maximumValue:               100
                                    value:                      neutralValue

                                    // Give slider 'center sprung' behavior
                                    onPressedChanged: {
                                        if (!slider.pressed) {
                                            slider.value = neutralValue
                                        }
                                        _lastIndex = index
                                    }
                                    // Disable mouse scroll
                                    MouseArea {
                                        anchors.fill: parent
                                        onWheel: {
                                            // do nothing
                                            wheel.accepted = true;
                                        }
                                        onPressed: {
                                            // propogate/accept
                                            mouse.accepted = false;
                                        }
                                        onReleased: {
                                            // propogate/accept
                                            mouse.accepted = false;
                                        }
                                    }
                                }
                            } // Column
                        } // Repeater
                    } // Row

                    QGCLabel {
                        width: parent.width
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           qsTr("Reverse Motor Direction")
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignBottom
                    }
                    Rectangle {
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 3
                        width:              parent.width
                        height:             1
                        color:              qgcPal.text
                    }

                    Row {
                        anchors.margins: ScreenTools.defaultFontPixelWidth

                        Repeater {
                            id:         cbRepeater
                            model:      controller.vehicle.motorCount == -1 ? 8 : controller.vehicle.motorCount

                            Column {
                                spacing:    ScreenTools.defaultFontPixelWidth

                                QGCCheckBox {
                                    width: sliderRow.width / (controller.vehicle.motorCount - 0.5)
                                    checked: controller.getParameterFact(-1, "MOT_" + (index + 1) + "_DIRECTION").value == -1
                                    onClicked: {
                                        sliderRepeater.itemAt(index).motorSlider.value = neutralValue
                                        setMotorDirection(index + 1, checked)
                                    }
                                }
                            } // Column
                        } // Repeater
                    } // Row
                } // Column

                // Display the frame currently in use with motor numbers
                APMSubMotorDisplay {
                    anchors.top:    parent.top
                    anchors.bottom: parent.bottom
                    width:          height
                    frameType: controller.getParameterFact(-1, "FRAME_CONFIG").value
                }
            } // Row

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("Moving the sliders will cause the motors to spin. Make sure the motors and propellers are clear from obstructions! The direction of the motor rotation is dependent on how the three phases of the motor are physically connected to the ESCs (if any two wires are swapped, the direction of rotation will flip). Because we cannot guarantee what order the phases are connected, the motor directions must be configured in software. When a slider is moved DOWN, the thruster should push air/water TOWARD the cable entering the housing. Click the checkbox to reverse the direction of the corresponding thruster.\n\n"
                                     + "Blue Robotics thrusters are lubricated by water and are not designed to be run in air. Testing the thrusters in air is ok at low speeds for short periods of time. Extended operation of Blue Robotics in air may lead to overheating and permanent damage. Without water lubrication, Blue Robotics thrusters may also make some unpleasant noises when operated in air; this is normal.")
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth
                Switch {
                    id: safetySwitch
                    onToggled: {
                        if (controller.vehicle.armed) {
                            timer.stop()
                            enabled = false
                            coolDownTimer.start()
                        }

                        controller.vehicle.armed = checked
                        checked = controller.vehicle.armed // Makes the switch stay off if it's not possible to arm
                    }
                }

                // Make sure external changes to Armed are reflected on the switch
                Connections {
                    target: controller.vehicle
                    onArmedChanged:
                    {
                        safetySwitch.checked = armed
                            if (!armed) {
                                timer.stop()
                                safetySwitch.enabled = false
                                coolDownTimer.start()
                            } else {
                                timer.start()
                            }
                            for (var sliderIndex=0; sliderIndex<sliderRepeater.count; sliderIndex++) {
                                sliderRepeater.itemAt(sliderIndex).motorSlider.value = neutralValue
                            }
                        }
                }

                QGCLabel {
                    anchors.verticalCenter: safetySwitch.verticalCenter
                    color:  qgcPal.warningText
                    text:   coolDownTimer.running
                                ? qsTr("A 10 second coooldown is required before testing again, please stand by...")
                                : qsTr("Slide this switch to arm the vehicle and enable the motor test (CAUTION!)")
                }
            } // Row

            QGCLabel {
                visible:             controller.vehicle.versionCompare(4, 0, 0) >= 0
                width:               parent.width
                anchors.left:        parent.left
                anchors.right:       parent.right
                font.pointSize:      ScreenTools.largeFontPointSize
                text:                qsTr("Automatic Motor Direction Detection")
            }

            QGCLabel {
                visible:        controller.vehicle.versionCompare(4, 0, 0) >= 0
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("This will attempt to automatically detect the direction (normal/reversed) of your thrusters.\n"
                                   + "Please place your vehicle in water, click the button, and wait. Note that the thrusters still need "
                                   + "to be connected to the correct outputs (thrusters 2 and 3 can't be swapped, for example).")
            }

            Row {
                visible:    controller.vehicle.versionCompare(4, 0, 0) >= 0
                spacing:    ScreenTools.defaultFontPixelWidth

                Column {
                    spacing:    ScreenTools.defaultFontPixelWidth * 2

                    QGCButton {
                        id: startAutoDetection
                        text: "Auto-Detect Directions"
                        enabled: controller.vehicle.flightMode !== 'Motor Detection'

                        onClicked: function() {
                            controller.vehicle.flightMode = "Motor Detection"
                            controller.vehicle.armed = true
                        }
                    }
                }
                Column {
                    spacing:    ScreenTools.defaultFontPixelWidth * 2

                    Flickable {
                        id: flickable
                        width: 500
                        height: Math.min(contentHeight, 200)
                        contentWidth: width
                        contentHeight: textArea.implicitHeight
                        clip: true

                        TextArea {
                            id: textArea
                            anchors.fill: parent
                            color:  qgcPal.text
                            text: controller.motorDetectionMessages
                            wrapMode: Text.WordWrap
                            background: Rectangle {
                                color: qgcPal.window
                            }
                            onTextChanged: function() {
                                flickable.flick(0, -300)
                            }
                        }
                        ScrollBar.vertical: ScrollBar {}
                    }
                }
            }

            // Repeats the command signal and updates the checkbox every 50 ms
            Timer {
                id: timer
                interval:       50
                repeat:         true
                running:        canDoManualTest

                onTriggered: {
                    if (controller.vehicle.armed) {
                            var slider = sliderRepeater.itemAt(_lastIndex)

                            var reversed = controller.getParameterFact(-1, "MOT_" + (_lastIndex + 1) + "_DIRECTION").value == -1

                            if (reversed) {
                                controller.vehicle.motorTest(_lastIndex, 100 - slider.motorSlider.value, 0)
                            } else {
                                controller.vehicle.motorTest(_lastIndex, slider.motorSlider.value, 0)
                            }
                    }
                }
            }
            Timer {
                id: coolDownTimer
                interval:       11000
                repeat:         false

                onTriggered: {
                    safetySwitch.enabled = true
                }
            }
        } // Column
    } // Component
} // SetupPahe
