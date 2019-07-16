/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

/// Joystick Config
SetupPage {
    id:                 joystickPage
    pageComponent:      pageComponent
    pageName:           qsTr("Joystick")
    pageDescription:    qsTr("Joystick Setup is used to configure a calibrate joysticks.")

    readonly property real  _maxButtons:         64
    readonly property real  _attitudeLabelWidth: ScreenTools.defaultFontPixelWidth * 12

    Connections {
        target: joystickManager
        onAvailableJoysticksChanged: {
            if(joystickManager.joysticks.length === 0) {
                summaryButton.checked = true
                setupView.showSummaryPanel()
            }
        }
    }

    Component {
        id: pageComponent
        Item {
            width:      availableWidth
            height:     mainColumn.height

            readonly property real  labelToMonitorMargin:   ScreenTools.defaultFontPixelWidth * 3
            property var            _activeJoystick:        joystickManager.activeJoystick

            function setupPageCompleted() {
                controller.start()
            }

            JoystickConfigController {
                id:             controller
                cancelButton:   cancelButton
                nextButton:     nextButton
                skipButton:     skipButton
            }

            // Main view
            Column {
                id:                     mainColumn
                spacing:                ScreenTools.defaultFontPixelHeight
                anchors.right:          parent.right
                anchors.left:           parent.left
                anchors.margins:        ScreenTools.defaultFontPixelWidth
                Row {
                    spacing:            ScreenTools.defaultFontPixelWidth * 2
                    QGCCheckBox {
                        id:                 enabledCheckBox
                        enabled:            _activeJoystick ? _activeJoystick.calibrated : false
                        text:               _activeJoystick ? _activeJoystick.calibrated ? qsTr("Enable joystick input") : qsTr("Enable not allowed (Calibrate First)") : ""
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked:          activeVehicle.joystickEnabled = checked
                        Component.onCompleted: {
                            checked = activeVehicle.joystickEnabled
                        }
                        Connections {
                            target: activeVehicle
                            onJoystickEnabledChanged: {
                                enabledCheckBox.checked = activeVehicle.joystickEnabled
                            }
                        }
                        Connections {
                            target: joystickManager
                            onActiveJoystickChanged: {
                                if(_activeJoystick) {
                                    enabledCheckBox.checked = Qt.binding(function() { return _activeJoystick.calibrated && activeVehicle.joystickEnabled })
                                }
                            }
                        }
                    }
                    QGCLabel {
                        text:           qsTr("Active joystick:")
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    QGCComboBox {
                        id:             joystickCombo
                        width:          ScreenTools.defaultFontPixelWidth * 40
                        anchors.verticalCenter: parent.verticalCenter
                        model:          joystickManager.joystickNames
                        onActivated:    joystickManager.activeJoystickName = textAt(index)
                        Component.onCompleted: {
                            var index = joystickCombo.find(joystickManager.activeJoystickName)
                            if (index === -1) {
                                console.warn(qsTr("Active joystick name not in combo"), joystickManager.activeJoystickName)
                            } else {
                                joystickCombo.currentIndex = index
                            }
                        }
                        Connections {
                            target: joystickManager
                            onAvailableJoysticksChanged: {
                                var index = joystickCombo.find(joystickManager.activeJoystickName)
                                if (index >= 0) {
                                    joystickCombo.currentIndex = index
                                }
                            }
                        }
                    }
                }
                //-- Separator
                Rectangle {
                    width:          parent.width
                    height:         1
                    border.color:   qgcPal.text
                    border.width:   1
                }
                //-- Main Row
                Row {
                    spacing:                    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    // Left side column
                    Column {
                        id:                     leftColumn
                        spacing:                ScreenTools.defaultFontPixelHeight

                        // Attitude Controls
                        Column {
                            width:      parent.width
                            spacing:    5

                            QGCLabel { text: qsTr("Attitude Controls") }

                            Item {
                                width:                  parent.width
                                height:                 defaultTextHeight * 2

                                QGCLabel {
                                    id:                 rollLabel
                                    width:              _attitudeLabelWidth
                                    text:               activeVehicle.sub ? qsTr("Lateral") : qsTr("Roll")
                                }

                                Loader {
                                    id:                 rollLoader
                                    anchors.left:       rollLabel.right
                                    anchors.right:      parent.right
                                    height:             ScreenTools.defaultFontPixelHeight
                                    width:              100
                                    sourceComponent:    axisMonitorDisplayComponent
                                    property bool mapped:   controller.rollAxisMapped
                                    property bool reversed: controller.rollAxisReversed
                                }

                                Connections {
                                    target:             _activeJoystick
                                    onManualControl:    rollLoader.item.axisValue = roll*32768.0
                                }
                            }

                            Item {
                                width:                  parent.width
                                height:                 defaultTextHeight * 2

                                QGCLabel {
                                    id:                 pitchLabel
                                    width:              _attitudeLabelWidth
                                    text:               activeVehicle.sub ? qsTr("Forward") : qsTr("Pitch")
                                }

                                Loader {
                                    id:                 pitchLoader
                                    anchors.left:       pitchLabel.right
                                    anchors.right:      parent.right
                                    height:             ScreenTools.defaultFontPixelHeight
                                    width:              100
                                    sourceComponent:    axisMonitorDisplayComponent
                                    property bool mapped:   controller.pitchAxisMapped
                                    property bool reversed: controller.pitchAxisReversed
                                }

                                Connections {
                                    target:             _activeJoystick
                                    onManualControl:    pitchLoader.item.axisValue = pitch*32768.0
                                }
                            }

                            Item {
                                width:                  parent.width
                                height:                 defaultTextHeight * 2

                                QGCLabel {
                                    id:                 yawLabel
                                    width:              _attitudeLabelWidth
                                    text:               qsTr("Yaw")
                                }

                                Loader {
                                    id:                 yawLoader
                                    anchors.left:       yawLabel.right
                                    anchors.right:      parent.right
                                    height:             ScreenTools.defaultFontPixelHeight
                                    width:              100
                                    sourceComponent:    axisMonitorDisplayComponent
                                    property bool mapped:   controller.yawAxisMapped
                                    property bool reversed: controller.yawAxisReversed
                                }

                                Connections {
                                    target:             _activeJoystick
                                    onManualControl:    yawLoader.item.axisValue = yaw*32768.0
                                }
                            }

                            Item {
                                width:                  parent.width
                                height:                 defaultTextHeight * 2

                                QGCLabel {
                                    id:                 throttleLabel
                                    width:              _attitudeLabelWidth
                                    text:               qsTr("Throttle")
                                }

                                Loader {
                                    id:                 throttleLoader
                                    anchors.left:       throttleLabel.right
                                    anchors.right:      parent.right
                                    height:             ScreenTools.defaultFontPixelHeight
                                    width:              100
                                    sourceComponent:    axisMonitorDisplayComponent
                                    property bool mapped:   controller.throttleAxisMapped
                                    property bool reversed: controller.throttleAxisReversed
                                }

                                Connections {
                                    target:             _activeJoystick
                                    onManualControl:    throttleLoader.item.axisValue = _activeJoystick.negativeThrust ? -throttle*32768.0 : (-2*throttle+1)*32768.0
                                }
                            }

                            Item {
                                width:                  parent.width
                                height:                 controller.hasGimbal ? defaultTextHeight * 2 : 0
                                visible:                controller.hasGimbal

                                QGCLabel {
                                    id:                 gimbalPitchLabel
                                    width:              _attitudeLabelWidth
                                    text:               qsTr("Gimbal Pitch")
                                }

                                Loader {
                                    id:                 gimbalPitchLoader
                                    anchors.left:       gimbalPitchLabel.right
                                    anchors.right:      parent.right
                                    height:             ScreenTools.defaultFontPixelHeight
                                    width:              100
                                    sourceComponent:    axisMonitorDisplayComponent
                                    property bool mapped:   controller.gimbalPitchAxisMapped
                                    property bool reversed: controller.gimbalPitchAxisReversed
                                }

                                Connections {
                                    target:             _activeJoystick
                                    onManualControl:    gimbalPitchLoader.item.axisValue = gimbalPitch * 32768.0
                                }
                            }

                            Item {
                                width:                  parent.width
                                height:                 controller.hasGimbal ? defaultTextHeight * 2 : 0
                                visible:                controller.hasGimbal

                                QGCLabel {
                                    id:                 gimbalYawLabel
                                    width:              _attitudeLabelWidth
                                    text:               qsTr("Gimbal Yaw")
                                }

                                Loader {
                                    id:                 gimbalYawLoader
                                    anchors.left:       gimbalYawLabel.right
                                    anchors.right:      parent.right
                                    height:             ScreenTools.defaultFontPixelHeight
                                    width:              100
                                    sourceComponent:    axisMonitorDisplayComponent
                                    property bool mapped:   controller.gimbalYawAxisMapped
                                    property bool reversed: controller.gimbalYawAxisReversed
                                }

                                Connections {
                                    target:             _activeJoystick
                                    onManualControl:    gimbalYawLoader.item.axisValue = gimbalYaw * 32768.0
                                }
                            }

                        } // Column - Attitude Control labels

                        // Command Buttons
                        Row {
                            spacing: 10
                            visible: _activeJoystick.requiresCalibration
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCButton {
                                id:         skipButton
                                text:       qsTr("Skip")
                                width:      ScreenTools.defaultFontPixelWidth * 10
                                onClicked:  controller.skipButtonClicked()
                            }
                            QGCButton {
                                id:         cancelButton
                                text:       qsTr("Cancel")
                                width:      ScreenTools.defaultFontPixelWidth * 10
                                onClicked:  controller.cancelButtonClicked()
                            }
                            QGCButton {
                                id:         nextButton
                                primary:    true
                                text:       qsTr("Calibrate")
                                width:      ScreenTools.defaultFontPixelWidth * 10
                                onClicked:  controller.nextButtonClicked()
                            }
                        }

                        // Status Text
                        QGCLabel {
                            text:           controller.statusText
                            width:          parent.width
                            wrapMode:       Text.WordWrap
                            visible:        text !== ""
                        }

                        //-- Separator
                        Rectangle {
                            width:          parent.width
                            height:         1
                            border.color:   qgcPal.text
                            border.width:   1
                        }

                        // Settings
                        Row {
                            width:      parent.width
                            spacing:    ScreenTools.defaultFontPixelWidth

                            // Left column settings
                            Column {
                                width:      parent.width * 0.5
                                spacing:    ScreenTools.defaultFontPixelHeight

                                QGCLabel { text: qsTr("Additional Joystick settings:") }

                                Column {
                                    width:      parent.width
                                    spacing:    ScreenTools.defaultFontPixelHeight

                                    Column {
                                        spacing: ScreenTools.defaultFontPixelHeight / 3
                                        visible: activeVehicle.supportsThrottleModeCenterZero

                                        QGCRadioButton {
                                            text:           qsTr("Center stick is zero throttle")
                                            checked:        _activeJoystick ? _activeJoystick.throttleMode === 0 : false

                                            onClicked: _activeJoystick.throttleMode = 0
                                        }

                                        Row {
                                            x:          20
                                            width:      parent.width
                                            spacing:    ScreenTools.defaultFontPixelWidth
                                            visible:    _activeJoystick ? _activeJoystick.throttleMode === 0 : false

                                            QGCCheckBox {
                                                id:         accumulator
                                                checked:    _activeJoystick ? _activeJoystick.accumulator : false
                                                text:       qsTr("Spring loaded throttle smoothing")

                                                onClicked:  _activeJoystick.accumulator = checked
                                            }
                                        }

                                        QGCRadioButton {
                                            text:           qsTr("Full down stick is zero throttle")
                                            checked:        _activeJoystick ? _activeJoystick.throttleMode === 1 : false

                                            onClicked: _activeJoystick.throttleMode = 1
                                        }

                                        QGCCheckBox {
                                            visible:        activeVehicle.supportsNegativeThrust
                                            id:             negativeThrust
                                            text:           qsTr("Allow negative Thrust")
                                            enabled:        _activeJoystick.negativeThrust = activeVehicle.supportsNegativeThrust
                                            checked:        _activeJoystick ? _activeJoystick.negativeThrust : false
                                            onClicked:      _activeJoystick.negativeThrust = checked
                                        }
                                    }

                                    Column {
                                        spacing: ScreenTools.defaultFontPixelHeight

                                        QGCLabel {
                                            id:                 expoSliderLabel
                                            text:               qsTr("Exponential:")
                                        }

                                        Row {
                                            spacing:            ScreenTools.defaultFontPixelWidth
                                            QGCSlider {
                                                id:             expoSlider
                                                width:          ScreenTools.defaultFontPixelWidth * 14
                                                minimumValue:   0
                                                maximumValue:   0.75
                                                Component.onCompleted: value = -_activeJoystick.exponential
                                                onValueChanged: _activeJoystick.exponential = -value
                                             }
                                            QGCLabel {
                                                id:     expoSliderIndicator
                                                text:   expoSlider.value.toFixed(2)
                                            }
                                        }
                                    }

                                    QGCCheckBox {
                                        id:         advancedSettings
                                        checked:    activeVehicle.joystickMode !== 0
                                        text:       qsTr("Advanced settings (careful!)")

                                        onClicked: {
                                            if (!checked) {
                                                activeVehicle.joystickMode = 0
                                            }
                                        }
                                    }

                                    Row {
                                        width:      parent.width
                                        spacing:    ScreenTools.defaultFontPixelWidth
                                        visible:    advancedSettings.checked

                                        QGCLabel {
                                            id:                 joystickModeLabel
                                            anchors.baseline:   joystickModeCombo.baseline
                                            text:               qsTr("Joystick mode:")
                                        }

                                        QGCComboBox {
                                            id:             joystickModeCombo
                                            currentIndex:   activeVehicle.joystickMode
                                            width:          ScreenTools.defaultFontPixelWidth * 20
                                            model:          activeVehicle.joystickModes

                                            onActivated: activeVehicle.joystickMode = index
                                        }
                                    }

                                    Row {
                                        width:      parent.width
                                        spacing:    ScreenTools.defaultFontPixelWidth
                                        visible:    advancedSettings.checked
                                        QGCLabel {
                                            text:       qsTr("Message frequency (Hz):")
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        QGCTextField {
                                            text:       _activeJoystick.frequency
                                            validator:  DoubleValidator { bottom: 0.25; top: 100.0; }
                                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            onEditingFinished: {
                                                _activeJoystick.frequency = parseFloat(text)
                                            }
                                        }
                                    }

                                    Row {
                                        width:      parent.width
                                        spacing:    ScreenTools.defaultFontPixelWidth
                                        visible:    advancedSettings.checked
                                        QGCCheckBox {
                                            id:         joystickCircleCorrection
                                            checked:    activeVehicle.joystickMode !== 0
                                            text:       qsTr("Enable circle correction")

                                            Component.onCompleted: checked = _activeJoystick.circleCorrection
                                            onClicked: {
                                                _activeJoystick.circleCorrection = checked
                                            }
                                        }
                                    }

                                    Row {
                                        width:      parent.width
                                        spacing:    ScreenTools.defaultFontPixelWidth
                                        visible:    advancedSettings.checked

                                        QGCCheckBox {
                                            id:         deadband
                                            checked:    controller.deadbandToggle
                                            text:       qsTr("Deadbands")

                                            onClicked:  controller.deadbandToggle = checked
                                        }
                                    }
                                    Row{
                                        width: parent.width
                                        spacing: ScreenTools.defaultFontPixelWidth
                                        visible: advancedSettings.checked
                                        QGCLabel{
                                            width:       parent.width * 0.85
                                            font.pointSize:     ScreenTools.smallFontPointSize
                                            wrapMode:           Text.WordWrap
                                            text:   qsTr("Deadband can be set during the first ") +
                                                    qsTr("step of calibration by gently wiggling each axis. ") +
                                                    qsTr("Deadband can also be adjusted by clicking and ") +
                                                    qsTr("dragging vertically on the corresponding axis monitor.")
                                            visible: controller.deadbandToggle
                                        }
                                    }
                                }
                            } // Column - left column

                            // Right column settings
                            Column {
                                width:      parent.width / 2
                                spacing:    ScreenTools.defaultFontPixelHeight

                                Connections {
                                    target: _activeJoystick

                                    onRawButtonPressedChanged: {
                                        if (buttonActionRepeater.itemAt(index)) {
                                            buttonActionRepeater.itemAt(index).pressed = pressed
                                        }
                                        if (jsButtonActionRepeater.itemAt(index)) {
                                            jsButtonActionRepeater.itemAt(index).pressed = pressed
                                        }
                                    }
                                }
                                //-- AP_JSButton Buttons (ArduSub)
                                QGCLabel { text: qsTr("Button actions:"); visible: activeVehicle.supportsJSButton; }
                                Column {
                                    width:      parent.width
                                    visible:    activeVehicle.supportsJSButton
                                    spacing:    ScreenTools.defaultFontPixelHeight / 3
                                    Row {
                                        spacing: ScreenTools.defaultFontPixelWidth
                                        QGCLabel {
                                            horizontalAlignment:    Text.AlignHCenter
                                            width:                  ScreenTools.defaultFontPixelHeight * 1.5
                                            text:                   qsTr("#")
                                        }
                                        QGCLabel {
                                            width:                  ScreenTools.defaultFontPixelWidth * 15
                                            text:                   qsTr("Function: ")
                                        }
                                        QGCLabel {
                                            width:                  ScreenTools.defaultFontPixelWidth * 15
                                            text:                   qsTr("Shift Function: ")
                                        }
                                    } // Row
                                    Repeater {
                                        id:     jsButtonActionRepeater
                                        model:  _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : 0

                                        Row {
                                            spacing: ScreenTools.defaultFontPixelWidth
                                            visible: activeVehicle.supportsJSButton

                                            property bool pressed

                                            Rectangle {
                                                anchors.verticalCenter:     parent.verticalCenter
                                                width:                      ScreenTools.defaultFontPixelHeight * 1.5
                                                height:                     width
                                                border.width:               1
                                                border.color:               qgcPal.text
                                                color:                      pressed ? qgcPal.buttonHighlight : qgcPal.button


                                                QGCLabel {
                                                    anchors.fill:           parent
                                                    color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                                                    horizontalAlignment:    Text.AlignHCenter
                                                    verticalAlignment:      Text.AlignVCenter
                                                    text:                   modelData
                                                }
                                            }

                                            FactComboBox {
                                                id:         mainJSButtonActionCombo
                                                width:      ScreenTools.defaultFontPixelWidth * 15
                                                fact:       controller.parameterExists(-1, "BTN"+index+"_FUNCTION") ? controller.getParameterFact(-1, "BTN" + index + "_FUNCTION") : null;
                                                indexModel: false
                                            }

                                            FactComboBox {
                                                id:         shiftJSButtonActionCombo
                                                width:      ScreenTools.defaultFontPixelWidth * 15
                                                fact:       controller.parameterExists(-1, "BTN"+index+"_SFUNCTION") ? controller.getParameterFact(-1, "BTN" + index + "_SFUNCTION") : null;
                                                indexModel: false
                                            }
                                        } // Row
                                    } // Repeater
                                } // Column
                            } // Column - right setting column
                        } // Row - Settings

                    } // Column - Left Main Column
                    // Right side column
                    Column {
                        width:          Math.min(ScreenTools.defaultFontPixelWidth * 35, availableWidth * 0.4)
                        spacing:        ScreenTools.defaultFontPixelHeight

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter

                            QGCLabel {
                                text: "TX Mode:"
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            QGCRadioButton {
                                text:       "1"
                                checked:    controller.transmitterMode == 1
                                enabled:    !controller.calibrating
                                onClicked:  controller.transmitterMode = 1
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            QGCRadioButton {
                                text:       "2"
                                checked:    controller.transmitterMode == 2
                                enabled:    !controller.calibrating
                                onClicked:  controller.transmitterMode = 2
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            QGCRadioButton {
                                text:       "3"
                                checked:    controller.transmitterMode == 3
                                enabled:    !controller.calibrating
                                onClicked:  controller.transmitterMode = 3
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            QGCRadioButton {
                                text:       "4"
                                checked:    controller.transmitterMode == 4
                                enabled:    !controller.calibrating
                                onClicked:  controller.transmitterMode = 4
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        //-------------------------------------------------------------
                        //-- Joystick Icon
                        Rectangle {
                            width:      Math.round(parent.width * 0.9)
                            height:     Math.round(width * 0.5)
                            radius:     ScreenTools.defaultFontPixelWidth * 2
                            color:      qgcPal.window
                            border.color: qgcPal.text
                            border.width: ScreenTools.defaultFontPixelWidth * 0.25
                            anchors.horizontalCenter: parent.horizontalCenter
                            property bool hasStickPositions: controller.stickPositions.length === 4
                            Connections {
                                target:     controller
                                onStickPositionsChanged: {
                                    console.log(controller.stickPositions[0] + ' ' + controller.stickPositions[1] + ' ' + controller.stickPositions[2] + ' ' + controller.stickPositions[3])
                                }
                            }

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
                            //---------------------------------------------------------
                            //-- Gimbal Pitch
                            Rectangle {
                                width:      ScreenTools.defaultFontPixelWidth * 0.125
                                height:     parent.height * 0.2
                                color:      qgcPal.text
                                visible:    controller.hasGimbal
                                x:          (parent.width  * 0.5) - (width  * 0.5)
                                y:          (parent.height * 0.5) - (height * 0.5)
                            }
                            Rectangle {
                                color:      qgcPal.colorGreen
                                width:      parent.width * 0.035
                                height:     width
                                radius:     width * 0.5
                                visible:    controller.hasGimbal
                                x:          (parent.width  * controller.gimbalPositions[0]) - (width  * 0.5)
                                y:          (parent.height * controller.gimbalPositions[1]) - (height * 0.5)
                            }
                            //---------------------------------------------------------
                            //-- Gimbal Yaw
                            Rectangle {
                                width:      parent.width * 0.2
                                height:     ScreenTools.defaultFontPixelWidth * 0.125
                                color:      qgcPal.text
                                visible:    controller.hasGimbal
                                x:          (parent.width  * 0.5) - (width  * 0.5)
                                y:          (parent.height * 0.3) - (height * 0.5)
                            }
                            Rectangle {
                                color:      qgcPal.colorGreen
                                width:      parent.width * 0.035
                                height:     width
                                radius:     width * 0.5
                                visible:    controller.hasGimbal
                                x:          (parent.width  * controller.gimbalPositions[2]) - (width  * 0.5)
                                y:          (parent.height * controller.gimbalPositions[3]) - (height * 0.5)
                            }
                        }

                        // Axis monitor
                        Column {
                            width:      parent.width
                            spacing:    5

                            QGCLabel {
                                text: qsTr("Axis Monitor")
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            Connections {
                                target: controller

                                onAxisValueChanged: {
                                    if (axisMonitorRepeater.itemAt(axis)) {
                                        axisMonitorRepeater.itemAt(axis).loader.item.axisValue = value
                                    }
                                }

                                onAxisDeadbandChanged: {
                                    if (axisMonitorRepeater.itemAt(axis)) {
                                        axisMonitorRepeater.itemAt(axis).loader.item.deadbandValue = value
                                    }
                                }
                            }

                            Repeater {
                                id:     axisMonitorRepeater
                                model:  _activeJoystick ? _activeJoystick.axisCount : 0
                                width:  parent.width

                                Row {
                                    spacing:    5
                                    anchors.horizontalCenter: parent.horizontalCenter

                                    // Need this to get to loader from Connections above
                                    property Item loader: theLoader

                                    QGCLabel {
                                        id:     axisLabel
                                        text:   modelData
                                    }

                                    Loader {
                                        id:                     theLoader
                                        anchors.verticalCenter: axisLabel.verticalCenter
                                        height:                 ScreenTools.defaultFontPixelHeight
                                        width:                  200
                                        sourceComponent:        axisMonitorDisplayComponent
                                        Component.onCompleted:  item.narrowIndicator = true

                                        property bool mapped:               true
                                        readonly property bool reversed:    false

                                        MouseArea {
                                            id:                 deadbandMouseArea
                                            anchors.fill:       parent.item
                                            enabled:            controller.deadbandToggle
                                            preventStealing:    true

                                            property real startX
                                            property real direction

                                            onPressed: {
                                                startX = mouseX
                                                direction = startX > width/2 ? 1 : -1
                                                parent.item.deadbandColor = "#3C6315"
                                            }
                                            onPositionChanged: {
                                                var mouseToDeadband = 32768/(width/2) // Factor to have deadband follow the mouse movement
                                                var newValue = parent.item.deadbandValue + direction*(mouseX - startX)*mouseToDeadband
                                                if ((newValue > 0) && (newValue <32768)){parent.item.deadbandValue=newValue;}
                                                startX = mouseX
                                            }
                                            onReleased: {
                                                controller.setDeadbandValue(modelData,parent.item.deadbandValue)
                                                parent.item.deadbandColor = "#8c161a"
                                            }
                                        }
                                    }

                                }
                            }
                        } // Column - Axis Monitor

                        // Button monitor
                        Column {
                            width:      parent.width
                            spacing:    ScreenTools.defaultFontPixelHeight

                            QGCLabel {
                                text: qsTr("Button Monitor")
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            Connections {
                                target: _activeJoystick

                                onRawButtonPressedChanged: {
                                    if (buttonMonitorRepeater.itemAt(index)) {
                                        buttonMonitorRepeater.itemAt(index).pressed = pressed
                                    }
                                }
                            }

                            Flow {
                                width:      parent.width * 0.9
                                spacing:    -1
                                anchors.horizontalCenter: parent.horizontalCenter

                                Repeater {
                                    id:     buttonMonitorRepeater
                                    model:  _activeJoystick ? _activeJoystick.totalButtonCount : 0

                                    Rectangle {
                                        width:          ScreenTools.defaultFontPixelHeight * 1.5
                                        height:         width
                                        border.width:   1
                                        border.color:   qgcPal.text
                                        color:          pressed ? qgcPal.buttonHighlight : qgcPal.windowShade

                                        property bool pressed

                                        QGCLabel {
                                            anchors.fill:           parent
                                            color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                                            horizontalAlignment:    Text.AlignHCenter
                                            verticalAlignment:      Text.AlignVCenter
                                            text:                   modelData
                                        }
                                    }
                                } // Repeater
                            } // Row
                        } // Column - Axis Monitor
                    } // Column - Right Column
                } // Main Row
                //-- Separator
                Rectangle {
                    width:          parent.width
                    height:         1
                    border.color:   qgcPal.text
                    border.width:   1
                }
                //-- Button Actions
                QGCLabel { text: qsTr("Button actions:"); visible: !activeVehicle.supportsJSButton; }
                Flow {
                    width:      parent.width
                    spacing:    ScreenTools.defaultFontPixelWidth
                    visible:    !activeVehicle.supportsJSButton
                    Repeater {
                        id:     buttonActionRepeater
                        model:  _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : []
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            visible: !activeVehicle.supportsJSButton
                            property bool pressed
                            QGCCheckBox {
                                anchors.verticalCenter:     parent.verticalCenter
                                checked:                    _activeJoystick ? _activeJoystick.buttonActions[modelData] !== "" : false
                                onClicked: _activeJoystick.setButtonAction(modelData, checked ? buttonActionCombo.textAt(buttonActionCombo.currentIndex) : "")
                            }
                            Rectangle {
                                anchors.verticalCenter:     parent.verticalCenter
                                width:                      ScreenTools.defaultFontPixelHeight * 1.5
                                height:                     width
                                border.width:               1
                                border.color:               qgcPal.text
                                color:                      pressed ? qgcPal.buttonHighlight : qgcPal.button
                                QGCLabel {
                                    anchors.fill:           parent
                                    color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                                    horizontalAlignment:    Text.AlignHCenter
                                    verticalAlignment:      Text.AlignVCenter
                                    text:                   modelData
                                }
                            }
                            QGCComboBox {
                                id:                         buttonActionCombo
                                width:                      ScreenTools.defaultFontPixelWidth * 20
                                model:                      _activeJoystick ? _activeJoystick.actions : 0

                                onActivated:                _activeJoystick.setButtonAction(modelData, textAt(index))
                                Component.onCompleted:      currentIndex = find(_activeJoystick.buttonActions[modelData])
                            }
                        }
                    }
                }
            }
            // Live axis monitor control component
            Component {
                id: axisMonitorDisplayComponent

                Item {
                    property int    axisValue:          0
                    property int    deadbandValue:      0
                    property bool   narrowIndicator:    false
                    property color  deadbandColor:      "#8c161a"

                    property color  __barColor:         qgcPal.windowShade

                    // Bar
                    Rectangle {
                        id:                     bar
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.width
                        height:                 parent.height / 2
                        color:                  __barColor
                    }

                    // Deadband
                    Rectangle {
                        id:                     deadbandBar
                        anchors.verticalCenter: parent.verticalCenter
                        x:                      _deadbandPosition
                        width:                  _deadbandWidth
                        height:                 parent.height / 2
                        color:                  deadbandColor
                        visible:                controller.deadbandToggle

                        property real _percentDeadband:     ((2 * deadbandValue) / (32768.0 * 2))
                        property real _deadbandWidth:       parent.width * _percentDeadband
                        property real _deadbandPosition:    (parent.width - _deadbandWidth) / 2
                    }

                    // Center point
                    Rectangle {
                        anchors.horizontalCenter:   parent.horizontalCenter
                        width:                      ScreenTools.defaultFontPixelWidth / 2
                        height:                     parent.height
                        color:                      qgcPal.window
                    }

                    // Indicator
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.narrowIndicator ?  height/6 : height
                        height:                 parent.height * 0.75
                        x:                      (reversed ? (parent.width - _indicatorPosition) : _indicatorPosition) - (width / 2)
                        radius:                 width / 2
                        color:                  qgcPal.text
                        visible:                mapped

                        property real _percentAxisValue:    ((axisValue + 32768.0) / (32768.0 * 2))
                        property real _indicatorPosition:   parent.width * _percentAxisValue
                    }

                    QGCLabel {
                        anchors.fill:           parent
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        text:                   qsTr("Not Mapped")
                        visible:                !mapped
                    }

                    ColorAnimation {
                        id:         barAnimation
                        target:     bar
                        property:   "color"
                        from:       "yellow"
                        to:         __barColor
                        duration:   1500
                    }


                    // Axis value debugger
                    /*
                    QGCLabel {
                        anchors.fill: parent
                        text: axisValue
                    }
                    */

                }
            } // Component - axisMonitorDisplayComponent
        } // Item
    } // Component - pageComponent
} // SetupPage


