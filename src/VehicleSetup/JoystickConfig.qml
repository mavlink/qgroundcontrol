/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.2
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


                    property int            __lastAxisValue:        0
                    readonly property int   __axisValueMaxJitter:   100
                    property color          __barColor:             qgcPal.windowShade

                    // Bar
                    Rectangle {
                        id:                     bar
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.width
                        height:                 parent.height / 2
                        color:                  __barColor
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
                        width:                  parent.height * 0.75
                        height:                 width
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

                    /*
                    // Axis value debugger
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
                            text:   qsTr("Roll")
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
                            target: controller

                            onRollAxisValueChanged: rollLoader.item.axisValue = value
                        }
                    }

                    Item {
                        width:  parent.width
                        height: defaultTextHeight * 2

                        QGCLabel {
                            id:     pitchLabel
                            width:  defaultTextWidth * 10
                            text:   qsTr("Pitch")
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
                            target: controller

                            onPitchAxisValueChanged: pitchLoader.item.axisValue = value
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
                            target: controller

                            onYawAxisValueChanged: yawLoader.item.axisValue = value
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
                            target: controller

                            onThrottleAxisValueChanged: throttleLoader.item.axisValue = value
                        }
                    }
                } // Column - Attitude Control labels

                // Command Buttons
                Row {
                    spacing: 10

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
                                enabled:    checked || _activeJoystick.calibrated
                                text:       _activeJoystick.calibrated ? qsTr("Enable joystick input") : qsTr("Enable not allowed (Calibrate First)")
                                checked:    _activeVehicle.joystickEnabled

                                onClicked:  _activeVehicle.joystickEnabled = checked
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
                                }
                            }

                            Column {
                                spacing: ScreenTools.defaultFontPixelHeight / 3
                                visible: _activeVehicle.supportsThrottleModeCenterZero

                                ExclusiveGroup { id: throttleModeExclusiveGroup }

                                QGCRadioButton {
                                    exclusiveGroup: throttleModeExclusiveGroup
                                    text:           qsTr("Center stick is zero throttle")
                                    checked:        _activeJoystick.throttleMode == 0

                                    onClicked: _activeJoystick.throttleMode = 0
                                }

                                QGCRadioButton {
                                    exclusiveGroup: throttleModeExclusiveGroup
                                    text:           qsTr("Full down stick is zero throttle")
                                    checked:        _activeJoystick.throttleMode == 1

                                    onClicked: _activeJoystick.throttleMode = 1
                                }
                            }

                            Column {
                                spacing: ScreenTools.defaultFontPixelHeight / 3

                                QGCCheckBox {
                                    id:         exponential
                                    checked:    _activeJoystick.exponential
                                    text:       qsTr("Use exponential curve on roll, pitch, yaw")

                                    onClicked:  _activeJoystick.exponential = checked
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

                            QGCLabel {
                                visible: _activeVehicle.manualControlReservedButtonCount != 0
                                text: qsTr("Buttons 0-%1 reserved for firmware use").arg(reservedButtonCount)

                                property int reservedButtonCount: _activeVehicle.manualControlReservedButtonCount == -1 ? _activeJoystick.totalButtonCount : _activeVehicle.manualControlReservedButtonCount
                            }

                            Repeater {
                                id:     buttonActionRepeater
                                model:  _activeJoystick.totalButtonCount

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    visible: (_activeVehicle.manualControlReservedButtonCount == -1 ? false : modelData >= _activeVehicle.manualControlReservedButtonCount) && !_activeVehicle.supportsJSButton

                                    property bool pressed

                                    QGCCheckBox {
                                        anchors.verticalCenter:     parent.verticalCenter
                                        checked:                    _activeJoystick.buttonActions[modelData] != ""

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
                                        model:          _activeJoystick.actions

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
                                model:  _activeJoystick.totalButtonCount

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
                width:          defaultTextWidth * 35
                spacing:        10

                Image {
                    //width:      parent.width
                    height:     defaultTextHeight * 15
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
                    }

                    Repeater {
                        id:     axisMonitorRepeater
                        model:  _activeJoystick.axisCount
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

                                property real defaultTextWidth:     ScreenTools.defaultFontPixelWidth
                                property bool mapped:               true
                                readonly property bool reversed:    false
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
                            model:  _activeJoystick.totalButtonCount

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
