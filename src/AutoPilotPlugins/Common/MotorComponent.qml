import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id:             motorPage
    pageComponent:  pageComponent

    property bool userLetterMotorIndices: false
    property MotorIdentificationController identifyController: MotorIdentificationController { }

    property bool _remapApplied: false

    readonly property int _barHeight:           10
    readonly property int _barWidth:            5
    readonly property int _sliderWidth:         15
    readonly property int _motorTimeoutSecs:    3

    function motorIndexToString(motorIndex) {
        let asciiA = 65;
        if (userLetterMotorIndices) {
            return String.fromCharCode(asciiA + motorIndex);
        } else {
            return motorIndex + 1;
        }
    }

    function identifyMotorName(motor) {
        return motorIndexToString(identifyController.motorTestNumber(motor) - 1)
    }

    function identifyStatusText() {
        if (identifyController.running) {
            return qsTr("Motor %1 is pulsing. Click the position on the diagram where the spinning motor is.").arg(identifyMotorName(identifyController.currentMotor))
        }
        if (!identifyController.complete) {
            if (_remapApplied) {
                return qsTr("Motor outputs were reassigned. Reboot the vehicle, then click Identify Motors again to confirm the motor order.")
            }
            return qsTr("Enable motors and set the throttle, then click Identify Motors. Each motor pulses in turn so you can click where it is on the diagram.")
        }
        if (!identifyController.layoutKnown) {
            return qsTr("Identification complete. Each position shows the motor that spun there.")
        }
        if (identifyController.orderCorrect) {
            return qsTr("All motors are in the expected positions.")
        }
        if (identifyController.remapChanges.length > 0) {
            return qsTr("Motors shown in red are not in the expected position. Apply the changes below to reassign the motor outputs to match where each motor is.")
        }
        return qsTr("Motors shown in red are not in the expected position. Check the frame class and type settings, and which flight controller output each motor is connected to.")
    }

    FactPanelController {
        id: controller
    }

    Component {
        id: pageComponent

        Column {
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel {
                text:       qsTr("Warning: Unable to determine motor count")
                color:      qgcPal.warningText
                visible:    controller.vehicle.motorCount == -1
            }

            Row {
                id:         motorSlider
                enabled:    safetySwitch.checked
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                ValueSlider {
                    id:                 sliderThrottle
                    width:              motorButtons.width
                    label:              qsTr("Throttle")
                    from:               0
                    to:                 100
                    majorTickStepSize:  5
                    decimalPlaces: 0
                    unitsString: qsTr("%")
                }
            } // Row

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("Make sure you remove all props.")
            }

            Row {
                id:         motorButtons
                enabled:    safetySwitch.checked
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                Repeater {
                    id:         buttonRepeater
                    model:      controller.vehicle.motorCount === -1 ? 8 : controller.vehicle.motorCount

                    QGCButton {
                        id:         button
                        anchors.verticalCenter:     parent.verticalCenter
                        text:       motorIndexToString(index)
                        enabled:    !identifyController.running
                        onClicked:  {
                            controller.vehicle.motorTest(index + 1, sliderThrottle.value, sliderThrottle.value === 0 ? 0 : _motorTimeoutSecs, true)
                        }
                    }
                } // Repeater

                QGCButton {
                    id:         allButton
                    text:       qsTr("All")
                    enabled:    !identifyController.running
                    onClicked:  {
                        for (var motorIndex=0; motorIndex<buttonRepeater.count; motorIndex++) {
                            controller.vehicle.motorTest(motorIndex + 1, sliderThrottle.value, sliderThrottle.value === 0 ? 0 : _motorTimeoutSecs, true)
                        }
                    }
                }

                QGCButton {
                    id:         allStopButton
                    text:       qsTr("Stop")
                    onClicked:  {
                        identifyController.cancel()
                        for (var motorIndex=0; motorIndex<buttonRepeater.count; motorIndex++) {
                            controller.vehicle.motorTest(motorIndex + 1, 0, 0, true)
                        }
                    }
                }
            } // Row

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                Switch {
                    id: safetySwitch
                    onClicked: {
                        if (!checked) {
                            identifyController.cancel()
                            sliderThrottle.setValue(0);
                        }
                    }
                }

                QGCLabel {
                    anchors.verticalCenter:     parent.verticalCenter
                    color:  qgcPal.warningText
                    text:   safetySwitch.checked ? qsTr("Careful : Motors are enabled") : qsTr("Propellers are removed - Enable slider and motors")
                }
            } // Row

            Column {
                id:         identifyColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2
                visible:    identifyController.supported

                readonly property real _positionSize:   ScreenTools.defaultFontPixelHeight * 3.5

                QGCLabel {
                    text:           qsTr("Motor Identification")
                    font.pointSize: ScreenTools.mediumFontPointSize
                }

                QGCLabel {
                    width:      frameDiagram.width * 2
                    wrapMode:   Text.WordWrap
                    text:       identifyStatusText()
                }

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 2

                    QGCButton {
                        text:       qsTr("Identify Motors")
                        enabled:    safetySwitch.checked && sliderThrottle.value > 0 && !identifyController.running
                        onClicked:  {
                            _remapApplied = false
                            identifyController.start(sliderThrottle.value)
                        }
                    }

                    QGCButton {
                        text:       qsTr("Cancel")
                        enabled:    identifyController.running
                        onClicked:  identifyController.cancel()
                    }
                }

                QGCLabel {
                    anchors.horizontalCenter:   frameDiagram.horizontalCenter
                    text:                       qsTr("Front")
                }

                Item {
                    id:     frameDiagram
                    width:  ScreenTools.defaultFontPixelHeight * 16
                    height: width

                    readonly property real _orbitRadius: (width - identifyColumn._positionSize) / 2

                    Rectangle {
                        anchors.centerIn:   parent
                        width:              identifyColumn._positionSize
                        height:             width
                        radius:             width / 4
                        color:              qgcPal.windowPanel
                        border.color:       qgcPal.borderSubtle

                        QGCColoredImage {
                            anchors.centerIn:   parent
                            width:              parent.width / 2
                            height:             width
                            source:             "/qmlimages/ArrowDirection.svg"
                            color:              qgcPal.textPrimary
                        }
                    }

                    Repeater {
                        model: identifyController.positionAngles

                        Rectangle {
                            readonly property int   _motor:     identifyController.selectedPositions.indexOf(index)
                            readonly property bool  _assigned:  _motor !== -1
                            readonly property bool  _wrong:     _assigned && identifyController.layoutKnown && _motor !== index
                            readonly property bool  _selectable: identifyController.running && !_assigned
                            readonly property real  _radians:   modelData * Math.PI / 180

                            x:              (frameDiagram.width / 2) + (frameDiagram._orbitRadius * Math.sin(_radians)) - (width / 2)
                            y:              (frameDiagram.height / 2) - (frameDiagram._orbitRadius * Math.cos(_radians)) - (height / 2)
                            width:          identifyColumn._positionSize
                            height:         width
                            radius:         width / 2
                            color:          _selectable && positionMouseArea.containsMouse ? qgcPal.accentSubtle : qgcPal.windowCard
                            border.width:   ScreenTools.defaultFontPixelWidth / 2
                            border.color:   _wrong ? qgcPal.colorRed : (_assigned ? qgcPal.colorGreen : (_selectable ? qgcPal.accent : qgcPal.borderSubtle))

                            Column {
                                anchors.centerIn: parent

                                QGCLabel {
                                    anchors.horizontalCenter:   parent.horizontalCenter
                                    text:                       _assigned ? identifyMotorName(_motor) : "?"
                                    font.pointSize:             ScreenTools.largeFontPointSize
                                }

                                QGCLabel {
                                    anchors.horizontalCenter:   parent.horizontalCenter
                                    text:                       qsTr("Expected %1").arg(identifyMotorName(index))
                                    font.pointSize:             ScreenTools.smallFontPointSize
                                    visible:                    _wrong
                                }
                            }

                            MouseArea {
                                id:             positionMouseArea
                                anchors.fill:   parent
                                hoverEnabled:   true
                                enabled:        _selectable
                                onClicked:      identifyController.selectPosition(index)
                            }
                        }
                    } // Repeater
                } // Item

                Repeater {
                    model: identifyController.remapChanges

                    QGCLabel {
                        text: modelData
                    }
                }

                QGCButton {
                    text:       qsTr("Apply Motor Order Fix")
                    visible:    identifyController.remapChanges.length > 0
                    enabled:    safetySwitch.checked
                    onClicked:  {
                        identifyController.remapMotors()
                        _remapApplied = true
                    }
                }
            } // Column
        } // Column
    } // Component
} // SetupPage
