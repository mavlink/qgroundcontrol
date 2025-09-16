/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl

import QGroundControl.Controls



import QGroundControl.FactControls

Item {
    height:                 calCol.height + ScreenTools.defaultFontPixelHeight * 2
    width:                  calCol.width  + ScreenTools.defaultFontPixelWidth  * 2
    Column {
        id:                 calCol
        spacing:            ScreenTools.defaultFontPixelHeight
        anchors.centerIn:   parent
        Item {
            height:         1
            width:          1
        }
        Row {
            spacing:            ScreenTools.defaultFontPixelWidth * 4
            anchors.horizontalCenter: parent.horizontalCenter
            //-----------------------------------------------------------------
            // Calibration
            Column {
                spacing:            ScreenTools.defaultFontPixelHeight
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    width:          Math.round(ScreenTools.defaultFontPixelWidth * 45)
                    height:         Math.round(width * 0.5)
                    radius:         ScreenTools.defaultFontPixelWidth * 2
                    color:          qgcPal.window
                    border.color:   qgcPal.text
                    border.width:   ScreenTools.defaultFontPixelWidth * 0.25
                    anchors.horizontalCenter: parent.horizontalCenter
                    property bool hasStickPositions: controller.stickPositions.length === 4
                    //---------------------------------------------------------
                    //-- Left Stick
                    Rectangle {
                        width:      parent.width * 0.25
                        height:     width
                        radius:     width * 0.5
                        color:      qgcPal.window
                        border.color: qgcPal.text
                        border.width: ScreenTools.defaultFontPixelWidth * 0.125
                        x:          (parent.width  * 0.25) - (width  * 0.5)
                        y:          (parent.height * 0.5)  - (height * 0.5)
                    }
                    Rectangle {
                        color:  qgcPal.colorGreen
                        width:  parent.width * 0.035
                        height: width
                        radius: width * 0.5
                        visible: parent.hasStickPositions
                        x:      (parent.width  * controller.stickPositions[0]) - (width  * 0.5)
                        y:      (parent.height * controller.stickPositions[1]) - (height * 0.5)
                    }
                    //---------------------------------------------------------
                    //-- Right Stick
                    Rectangle {
                        width:      parent.width * 0.25
                        height:     width
                        radius:     width * 0.5
                        color:      qgcPal.window
                        border.color: qgcPal.text
                        border.width: ScreenTools.defaultFontPixelWidth * 0.125
                        x:          (parent.width  * 0.75) - (width  * 0.5)
                        y:          (parent.height * 0.5)  - (height * 0.5)
                    }
                    Rectangle {
                        color:  qgcPal.colorGreen
                        width:  parent.width * 0.035
                        height: width
                        radius: width * 0.5
                        visible: parent.hasStickPositions
                        x:      (parent.width  * controller.stickPositions[2]) - (width  * 0.5)
                        y:      (parent.height * controller.stickPositions[3]) - (height * 0.5)
                    }
                }
            }
            //---------------------------------------------------------------------
            // Monitor
            Column {
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.verticalCenter: parent.verticalCenter
                Connections {
                    target: controller
                    onAxisValueChanged: (axis, value) => {
                        if (axisMonitorRepeater.itemAt(axis)) {
                            axisMonitorRepeater.itemAt(axis).axis.axisValue = value
                        }
                    }
                    onAxisDeadbandChanged: (axis, value) => {
                        if (axisMonitorRepeater.itemAt(axis)) {
                            axisMonitorRepeater.itemAt(axis).axis.deadbandValue = value
                        }
                    }
                }

                Repeater {
    id:     axisMonitorRepeater
    model:  _activeJoystick ? _activeJoystick.axisCount : 0
    width:  parent.width

    Row {
        spacing: 5
        anchors.horizontalCenter: parent.horizontalCenter

        // 👇 restore this so Connections can find ".axis"
        property Item axis: theAxis
        property int  axisIndex: modelData

        QGCLabel {
            id:   axisLabel
            text: modelData
        }

        AxisMonitor {
            id:                     theAxis
            anchors.verticalCenter: axisLabel.verticalCenter
            height:                 ScreenTools.defaultFontPixelHeight
            width:                  200
            narrowIndicator:        true
            mapped:                 true
            reversed:               false

            // (optional) raw range accessors
            property int rawMin:    _activeJoystick ? _activeJoystick.axisRawMin(axisIndex)    : 0
            property int rawMax:    _activeJoystick ? _activeJoystick.axisRawMax(axisIndex)    : 32767
            property int rawCenter: _activeJoystick ? _activeJoystick.axisRawCenter(axisIndex) : Math.round((rawMin + rawMax)/2)
            property int halfRange: Math.max(rawCenter - rawMin, rawMax - rawCenter)

            MouseArea {
                id:                 deadbandMouseArea
                anchors.fill:       parent.item
                enabled:            controller.deadbandToggle
                preventStealing:    true
                property real startX
                property real direction

                onPressed: (mouse) => {
                    startX    = mouse.x
                    direction = startX > width/2 ? 1 : -1
                    parent.item.deadbandColor = "#3C6315"
                }
                onPositionChanged: (mouse) => {
                    var unitsPerPixel = theAxis.halfRange / (theAxis.width / 2)
                    var deltaUnits    = direction * (mouse.x - startX) * unitsPerPixel
                    var newValue      = parent.item.deadbandValue + deltaUnits
                    newValue = Math.max(0, Math.min(theAxis.halfRange, newValue))
                    parent.item.deadbandValue = newValue
                    startX = mouse.x
                }
                onReleased: {
                    controller.setDeadbandValue(axisIndex, parent.item.deadbandValue)
                    parent.item.deadbandColor = "#8c161a"
                }
            }
        }
    }
}

                // Repeater {
                //     id:             axisMonitorRepeater
                //     model:          _activeJoystick ? _activeJoystick.axisCount : 0
                //     width:          parent.width
                //     Row {
                //         spacing:    5
                //         anchors.horizontalCenter: parent.horizontalCenter
                //         // Need this to get to loader from Connections above
                //         property Item axis: theAxis
                //         QGCLabel {
                //             id:     axisLabel
                //             text:   modelData
                //         }
                //         AxisMonitor {
                //             id:                     theAxis
                //             anchors.verticalCenter: axisLabel.verticalCenter
                //             height:                 ScreenTools.defaultFontPixelHeight
                //             width:                  200
                //             narrowIndicator:        true
                //             mapped:                 true
                //             reversed:               false
                //             MouseArea {
                //                 id:                 deadbandMouseArea
                //                 anchors.fill:       parent.item
                //                 enabled:            controller.deadbandToggle
                //                 preventStealing:    true
                //                 property real startX
                //                 property real direction
                //                 onPressed: (mouse) => {
                //                     startX = mouseX
                //                     direction = startX > width/2 ? 1 : -1
                //                     parent.item.deadbandColor = "#3C6315"
                //                 }
                //                 onPositionChanged: {
                //                     var mouseToDeadband = 32768/(width/2) // Factor to have deadband follow the mouse movement
                //                     var newValue = parent.item.deadbandValue + direction*(mouseX - startX)*mouseToDeadband
                //                     if ((newValue > 0) && (newValue <32768)){parent.item.deadbandValue=newValue;}
                //                     startX = mouseX
                //                 }
                //                 onReleased: {
                //                     controller.setDeadbandValue(modelData,parent.item.deadbandValue)
                //                     parent.item.deadbandColor = "#8c161a"
                //                 }
                //             }
                //         }
                //     }
                // }
            }
        }
        // Command Buttons
        Row {
            spacing:            ScreenTools.defaultFontPixelWidth * 2
            visible:            _activeJoystick.requiresCalibration
            anchors.horizontalCenter: parent.horizontalCenter
            QGCButton {
                id:         skipButton
                text:       qsTr("Skip")
                enabled:    controller.calibrating ? controller.skipEnabled : false
                width:      ScreenTools.defaultFontPixelWidth * 10
                onClicked:  controller.skipButtonClicked()
            }
            QGCButton {
                text:       qsTr("Cancel")
                width:      ScreenTools.defaultFontPixelWidth * 10
                enabled:    controller.calibrating
                onClicked: {
                    if(controller.calibrating)
                        controller.cancelButtonClicked()
                }
            }
            QGCButton {
                id:         nextButton
                primary:    true
                enabled:    controller.calibrating ? controller.nextEnabled : true
                text:       controller.calibrating ? qsTr("Next") : qsTr("Start")
                width:      ScreenTools.defaultFontPixelWidth * 10
                onClicked:  controller.nextButtonClicked()
            }
        }
        // Status Text
        QGCLabel {
            text:           controller.statusText
            width:          parent.width * 0.8
            wrapMode:       Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            width: parent.width * 0.9
            height: 100
            color: "transparent"
            radius: 8
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: ScreenTools.defaultFontPixelHeight

            Column {
                anchors.margins: ScreenTools.defaultFontPixelHeight
                spacing: ScreenTools.defaultFontPixelHeight * 0.5

                QGCLabel {
                    text: qsTr("Gimbal Control Info")
                    font.bold: true
                }
                QGCLabel {
                    text: qsTr("Axis 5 = Gimbal Pitch   |   Axis 4 = Gimbal Yaw")
                    color: qgcPal.text
                }

                // Gimbal Deadzone Axis 4 (Yaw)
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 2
                    QGCLabel {
                        text: qsTr("Gimbal Yaw Deadzone (Axis 4):")
                        width: 220
                    }
                    Slider {
                        id: yawDeadzoneSlider
                        width: 150
                        from: 0
                        to: 100
                        stepSize: 1
                        value: _activeJoystick ? _activeJoystick.gimbalYawDeadzone : 0
                        onValueChanged: if (_activeJoystick) _activeJoystick.gimbalYawDeadzone = value
                    }
                    QGCLabel { text: yawDeadzoneSlider.value.toFixed(0) }
                }

                // Gimbal Deadzone Axis 5 Pitch
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 2
                    QGCLabel {
                        text: qsTr("Gimbal Pitch Deadzone (Axis 5):")
                        width: 220
                    }
                    Slider {
                        id: pitchDeadzoneSlider
                        width: 150
                        from: 0
                        to: 100
                        stepSize: 1
                        value: _activeJoystick ? _activeJoystick.gimbalPitchDeadzone : 0
                        onValueChanged: if (_activeJoystick) _activeJoystick.gimbalPitchDeadzone = value
                    }
                    QGCLabel { text: pitchDeadzoneSlider.value.toFixed(0) }
                }

                // Gimbal Speed
               Row {
    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        text: qsTr("Gimbal Max Speed:")
        width: 220
    }

    QGCTextField {
        id: speedField
        width: 150
        enabled: _activeJoystick !== null
        placeholderText: qsTr("0–100")
        text: _activeJoystick ? String(_activeJoystick.gimbalMaxSpeed) : "0"
        inputMethodHints: Qt.ImhDigitsOnly
        validator: IntValidator { bottom: 0; top: 100 }

        // Commit on Enter or when the field loses focus
        onAccepted: {
            if (_activeJoystick) _activeJoystick.gimbalMaxSpeed = parseInt(text)
        }
        onEditingFinished: {
            if (_activeJoystick) _activeJoystick.gimbalMaxSpeed = parseInt(text)
        }
    }

    QGCLabel {
        text: qsTr("deg/s")
    }
}


                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 2
                    QGCLabel {
                        text: qsTr("Gimbal Axis Control:")
                        width: 220
                    }
                    Switch {
                        id: gimbalAxisSwitch
                        checked: _activeJoystick ? _activeJoystick.gimbalAxisEnabled : true
                        onToggled: if (_activeJoystick) _activeJoystick.gimbalAxisEnabled = checked
                    }
                    QGCLabel {
                        text: gimbalAxisSwitch.checked ? qsTr("Enabled") : qsTr("Disabled")
                    }
                }

            }
        } 
    }
}


