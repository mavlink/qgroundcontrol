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

    readonly property real _maxButtons: 16

    Connections {
        target: joystickManager
        onAvailableJoysticksChanged: {
            if( joystickManager.joysticks.length == 0 ) {
                summaryButton.checked = true
                setupView.showSummaryPanel()
            }
        }
    }

    Component {
        id: pageComponent

        Item {
            width:  availableWidth
            height: Math.max(leftColumn.height, rightColumn.height)

            property bool controllerCompleted:      false
            property bool controllerAndViewReady:   false

            readonly property real labelToMonitorMargin: defaultTextWidth * 3

            property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
            property var _activeJoystick:   joystickManager.activeJoystick

            JoystickConfigController {
                id:             controller
                factPanel:      joystickPage.viewPanel
                statusText:     statusText
                cancelButton:   cancelButton
                nextButton:     nextButton
                skipButton:     skipButton

                Component.onCompleted: {
                    controllerCompleted = true
                    if (joystickPage.completedSignalled) {
                        controllerAndViewReady = true
                        controller.start()
                    }
                }
            }

            Component.onCompleted: {
                if (controllerCompleted) {
                    controllerAndViewReady = true
                    controller.start()
                }
            }

            // Live axis monitor control component
            Component {
                id: axisMonitorDisplayComponent

                Item {
                    property int axisValue: 0
                    property int deadbandValue: 0
                    property bool narrowIndicator: false
                    property color deadbandColor: "#8c161a"

                    property color          __barColor:             qgcPal.windowShade

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

                        property real _percentDeadband:    ((2 * deadbandValue) / (32768.0 * 2))
                        property real _deadbandWidth:   parent.width * _percentDeadband
                        property real _deadbandPosition:   (parent.width - _deadbandWidth) / 2
                    }

                    // Center point
                    Rectangle {
                        anchors.horizontalCenter:   parent.horizontalCenter
                        width:                      defaultTextWidth / 2
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

            // Main view Qml starts here

            // Left side column
            Column {
                id:                     leftColumn
                anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                anchors.left:           parent.left
                anchors.right:          rightColumn.left
                spacing:                10

                // Attitude Controls
                Column {
                    width:      parent.width
                    spacing:    5

                    QGCLabel { text: qsTr("Attitude Controls") }

                    Item {
                        width:  parent.width
                        height: defaultTextHeight * 2

                        QGCLabel {
                            id:     rollLabel
                            width:  defaultTextWidth * 10
                            text:   _activeVehicle.sub ? qsTr("Lateral") : qsTr("Roll")
                        }

                        Loader {
                            id:                 rollLoader
                            anchors.left:       rollLabel.right
                            anchors.right:      parent.right
                            height:             ScreenTools.defaultFontPixelHeight
                            width:              100
                            sourceComponent:    axisMonitorDisplayComponent

                            property real defaultTextWidth: ScreenTools.defaultFontPixelWidth
                            property bool mapped:           controller.rollAxisMapped
                            property bool reversed:         controller.rollAxisReversed
                        }

                        Connections {
                            target: _activeJoystick

                            onManualControl: rollLoader.item.axisValue = roll*32768.0
                        }
                    }

                    Item {
                        width:  parent.width
                        height: defaultTextHeight * 2

                        QGCLabel {
                            id:     pitchLabel
                            width:  defaultTextWidth * 10
                            text:   _activeVehicle.sub ? qsTr("Forward") : qsTr("Pitch")
                        }

                        Loader {
                            id:                 pitchLoader
                            anchors.left:       pitchLabel.right
                            anchors.right:      parent.right
                            height:             ScreenTools.defaultFontPixelHeight
                            width:              100
                            sourceComponent:    axisMonitorDisplayComponent

                            property real defaultTextWidth: ScreenTools.defaultFontPixelWidth
                            property bool mapped:           controller.pitchAxisMapped
                            property bool reversed:         controller.pitchAxisReversed
                        }

                        Connections {
                            target: _activeJoystick

                            onManualControl: pitchLoader.item.axisValue = pitch*32768.0
                        }
                    }

                    Item {
                        width:  parent.width
                        height: defaultTextHeight * 2

                        QGCLabel {
                            id:     yawLabel
                            width:  defaultTextWidth * 10
                            text:   qsTr("Yaw")
                        }

                        Loader {
                            id:                 yawLoader
                            anchors.left:       yawLabel.right
                            anchors.right:      parent.right
                            height:             ScreenTools.defaultFontPixelHeight
                            width:              100
                            sourceComponent:    axisMonitorDisplayComponent

                            property real defaultTextWidth: ScreenTools.defaultFontPixelWidth
                            property bool mapped:           controller.yawAxisMapped
                            property bool reversed:         controller.yawAxisReversed
                        }

                        Connections {
                            target: _activeJoystick

                            onManualControl: yawLoader.item.axisValue = yaw*32768.0
                        }
                    }

                    Item {
                        width:  parent.width
                        height: defaultTextHeight * 2

                        QGCLabel {
                            id:     throttleLabel
                            width:  defaultTextWidth * 10
                            text:   qsTr("Throttle")
                        }

                        Loader {
                            id:                 throttleLoader
                            anchors.left:       throttleLabel.right
                            anchors.right:      parent.right
                            height:             ScreenTools.defaultFontPixelHeight
                            width:              100
                            sourceComponent:    axisMonitorDisplayComponent

                            property real defaultTextWidth: ScreenTools.defaultFontPixelWidth
                            property bool mapped:           controller.throttleAxisMapped
                            property bool reversed:         controller.throttleAxisReversed
                        }

                        Connections {
                            target: _activeJoystick

                            onManualControl: throttleLoader.item.axisValue = _activeJoystick.negativeThrust ? -throttle*32768.0 : (-2*throttle+1)*32768.0
                        }
                    }
                } // Column - Attitude Control labels

                // Command Buttons
                Row {
                    spacing: 10
                    visible: _activeJoystick.requiresCalibration

                    QGCButton {
                        id:     skipButton
                        text:   qsTr("Skip")

                        onClicked: controller.skipButtonClicked()
                    }

                    QGCButton {
                        id:     cancelButton
                        text:   qsTr("Cancel")

                        onClicked: controller.cancelButtonClicked()
                    }

                    QGCButton {
                        id:         nextButton
                        primary:    true
                        text:       qsTr("Calibrate")

                        onClicked: controller.nextButtonClicked()
                    }
                } // Row - Buttons

                // Status Text
                QGCLabel {
                    id:         statusText
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                }

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
                        width:      parent.width / 2
                        spacing:    ScreenTools.defaultFontPixelHeight

                        QGCLabel { text: qsTr("Additional Joystick settings:") }

                        Column {
                            width:      parent.width
                            spacing:    ScreenTools.defaultFontPixelHeight


                            QGCCheckBox {
                                id:         enabledCheckBox
                                enabled:    _activeJoystick ? _activeJoystick.calibrated : false
                                text:       _activeJoystick ? _activeJoystick.calibrated ? qsTr("Enable joystick input") : qsTr("Enable not allowed (Calibrate First)") : ""
                                onClicked:  _activeVehicle.joystickEnabled = checked
                                Component.onCompleted: checked = _activeVehicle.joystickEnabled

                                Connections {
                                    target: _activeVehicle

                                    onJoystickEnabledChanged: {
                                        enabledCheckBox.checked = _activeVehicle.joystickEnabled
                                    }
                                }

                                Connections {
                                    target: joystickManager

                                    onActiveJoystickChanged: {
                                        if(_activeJoystick) {
                                            enabledCheckBox.checked = Qt.binding(function() { return _activeJoystick.calibrated && _activeVehicle.joystickEnabled })
                                        }
                                    }
                                }
                            }

                            Row {
                                width:      parent.width
                                spacing:    ScreenTools.defaultFontPixelWidth

                                QGCLabel {
                                    id:                 activeJoystickLabel
                                    anchors.baseline:   joystickCombo.baseline
                                    text:               qsTr("Active joystick:")
                                }

                                QGCComboBox {
                                    id:                 joystickCombo
                                    width:              parent.width - activeJoystickLabel.width - parent.spacing
                                    model:              joystickManager.joystickNames

                                    onActivated: joystickManager.activeJoystickName = textAt(index)

                                    Component.onCompleted: {
                                        var index = joystickCombo.find(joystickManager.activeJoystickName)
                                        if (index == -1) {
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

                            Column {
                                spacing: ScreenTools.defaultFontPixelHeight / 3
                                visible: _activeVehicle.supportsThrottleModeCenterZero

                                ExclusiveGroup { id: throttleModeExclusiveGroup }

                                QGCRadioButton {
                                    exclusiveGroup: throttleModeExclusiveGroup
                                    text:           qsTr("Center stick is zero throttle")
                                    checked:        _activeJoystick ? _activeJoystick.throttleMode == 0 : false

                                    onClicked: _activeJoystick.throttleMode = 0
                                }

                                Row {
                                    x:          20
                                    width:      parent.width
                                    spacing:    ScreenTools.defaultFontPixelWidth
                                    visible:    _activeJoystick ? _activeJoystick.throttleMode == 0 : false

                                    QGCCheckBox {
                                        id:         accumulator
                                        checked:    _activeJoystick ? _activeJoystick.accumulator : false
                                        text:       qsTr("Spring loaded throttle smoothing")

                                        onClicked:  _activeJoystick.accumulator = checked
                                    }
                                }

                                QGCRadioButton {
                                    exclusiveGroup: throttleModeExclusiveGroup
                                    text:           qsTr("Full down stick is zero throttle")
                                    checked:        _activeJoystick ? _activeJoystick.throttleMode == 1 : false

                                    onClicked: _activeJoystick.throttleMode = 1
                                }

                                QGCCheckBox {
                                    visible:        _activeVehicle.supportsNegativeThrust
                                    id:             negativeThrust
                                    text:           qsTr("Allow negative Thrust")
                                    enabled:        _activeJoystick.negativeThrust = _activeVehicle.supportsNegativeThrust
                                    checked:        _activeJoystick ? _activeJoystick.negativeThrust : false
                                    onClicked:      _activeJoystick.negativeThrust = checked
                                }
                            }

                            Column {
                                spacing: ScreenTools.defaultFontPixelHeight / 3

                                QGCLabel {
                                    id:                 expoSliderLabel
                                    text:               qsTr("Exponential:")
                                }

                                Row {
                                    QGCSlider {
                                        id: expoSlider
                                        minimumValue: 0
                                        maximumValue: 0.75

                                        Component.onCompleted: value=-_activeJoystick.exponential
                                        onValueChanged: _activeJoystick.exponential=-value
                                     }

                                    QGCLabel {
                                        id:     expoSliderIndicator
                                        text:   expoSlider.value.toFixed(2)
                                    }
                                }
                            }

                            QGCCheckBox {
                                id:         advancedSettings
                                checked:    _activeVehicle.joystickMode != 0
                                text:       qsTr("Advanced settings (careful!)")

                                onClicked: {
                                    if (!checked) {
                                        _activeVehicle.joystickMode = 0
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
                                    currentIndex:   _activeVehicle.joystickMode
                                    width:          ScreenTools.defaultFontPixelWidth * 20
                                    model:          _activeVehicle.joystickModes

                                    onActivated: _activeVehicle.joystickMode = index
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
                                    checked:    _activeVehicle.joystickMode != 0
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

                        QGCLabel { text: qsTr("Button actions:") }

                        Column {
                            width:      parent.width
                            spacing:    ScreenTools.defaultFontPixelHeight / 3

                            Repeater {
                                id:     buttonActionRepeater
                                model:  _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : 0

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    visible: !_activeVehicle.supportsJSButton

                                    property bool pressed

                                    QGCCheckBox {
                                        anchors.verticalCenter:     parent.verticalCenter
                                        checked:                    _activeJoystick ? _activeJoystick.buttonActions[modelData] != "" : false

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
                                        id:             buttonActionCombo
                                        width:          ScreenTools.defaultFontPixelWidth * 20
                                        model:          _activeJoystick ? _activeJoystick.actions : 0

                                        onActivated:            _activeJoystick.setButtonAction(modelData, textAt(index))
                                        Component.onCompleted:  currentIndex = find(_activeJoystick.buttonActions[modelData])
                                    }
                                }
                            } // Repeater

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                visible: _activeVehicle.supportsJSButton

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
                                    visible: _activeVehicle.supportsJSButton

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
                id:             rightColumn
                anchors.top:    parent.top
                anchors.right:  parent.right
                width:          Math.min(joystickPage.defaultTextWidth * 35, availableWidth * 0.4)
                spacing:        ScreenTools.defaultFontPixelHeight / 2

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    ExclusiveGroup { id: modeGroup }

                    QGCLabel {
                        text: "TX Mode:"
                    }

                    QGCRadioButton {
                        exclusiveGroup: modeGroup
                        text:           "1"
                        checked:        controller.transmitterMode == 1
                        enabled:        !controller.calibrating

                        onClicked: controller.transmitterMode = 1
                    }

                    QGCRadioButton {
                        exclusiveGroup: modeGroup
                        text:           "2"
                        checked:        controller.transmitterMode == 2
                        enabled:        !controller.calibrating

                        onClicked: controller.transmitterMode = 2
                    }

                    QGCRadioButton {
                        exclusiveGroup: modeGroup
                        text:           "3"
                        checked:        controller.transmitterMode == 3
                        enabled:        !controller.calibrating

                        onClicked: controller.transmitterMode = 3
                    }

                    QGCRadioButton {
                        exclusiveGroup: modeGroup
                        text:           "4"
                        checked:        controller.transmitterMode == 4
                        enabled:        !controller.calibrating

                        onClicked: controller.transmitterMode = 4
                    }
                }

                Image {
                    width:      parent.width
                    fillMode:   Image.PreserveAspectFit
                    smooth:     true
                    source:     controller.imageHelp
                }

                // Axis monitor
                Column {
                    width:      parent.width
                    spacing:    5

                    QGCLabel { text: qsTr("Axis Monitor") }

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

                                property real defaultTextWidth:     ScreenTools.defaultFontPixelWidth
                                property bool mapped:               true
                                readonly property bool reversed:    false


                                MouseArea {
                                    id:             deadbandMouseArea
                                    anchors.fill:   parent.item
                                    enabled:        controller.deadbandToggle

                                    property real startY

                                    onPressed: {
                                        startY = mouseY
                                        parent.item.deadbandColor = "#3C6315"
                                    }
                                    onPositionChanged: {
                                        var newValue = parent.item.deadbandValue + (startY - mouseY)*15
                                        if ((newValue > 0) && (newValue <32768)){parent.item.deadbandValue=newValue;}
                                        startY = mouseY
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

                    QGCLabel { text: qsTr("Button Monitor") }

                    Connections {
                        target: _activeJoystick

                        onRawButtonPressedChanged: {
                            if (buttonMonitorRepeater.itemAt(index)) {
                                buttonMonitorRepeater.itemAt(index).pressed = pressed
                            }
                        }
                    }

                    Flow {
                        width:      parent.width
                        spacing:    -1

                        Repeater {
                            id:     buttonMonitorRepeater
                            model:  _activeJoystick ? _activeJoystick.totalButtonCount : 0

                            Rectangle {
                                width:          ScreenTools.defaultFontPixelHeight * 1.2
                                height:         width
                                border.width:   1
                                border.color:   qgcPal.text
                                color:          pressed ? qgcPal.buttonHighlight : qgcPal.button

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
        } // Item
    } // Component - pageComponent
} // SetupPage


