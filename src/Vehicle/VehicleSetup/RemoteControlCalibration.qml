import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

/// Base class for Remote Control Calibration (supports both RC and Joystick)
RowLayout {
    property Component additionalSetupComponent

    id: root
    spacing: ScreenTools.defaultFontPixelWidth

    property var _controller: controller

    ListModel {
        id: attitudeControlsModel

        Component.onCompleted: {
            attitudeControlsModel.append(
                {
                    name: qsTr("Roll"),
                    mapped: controller.rollChannelMapped,
                    reversed: controller.rollChannelReversed,
                    value: controller.rollChannelValue
                })
            attitudeControlsModel.append(
                {
                    name: qsTr("Pitch"),
                    mapped: controller.pitchChannelMapped,
                    reversed: controller.pitchChannelReversed,
                    value: controller.pitchChannelValue
                })
            attitudeControlsModel.append(
                {
                    name: qsTr("Yaw"),
                    mapped: controller.yawChannelMapped,
                    reversed: controller.yawChannelReversed,
                    value: controller.yawChannelValue
                })
            attitudeControlsModel.append(
                {
                    name: qsTr("Throttle"),
                    mapped: controller.throttleChannelMapped,
                    reversed: controller.throttleChannelReversed,
                    value: controller.throttleChannelValue
                })
        }
    }

    function setupPageCompleted() {
        controller.start()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    RadioComponentController {
        id: controller
        statusText: statusText
        cancelButton: cancelButton
        nextButton: nextButton
        skipButton: skipButton
        onThrottleReversedCalFailure: mainWindow.showMessageDialog(qsTr("Throttle channel reversed"), qsTr("Calibration failed. The throttle channel on your transmitter is reversed. You must correct this on your transmitter in order to complete calibration."))
    }

    ColumnLayout {
        id: leftColumnLayout
        Layout.alignment: Qt.AlignTop
        spacing: 10

        ColumnLayout {
            id: attitudeControlsLayout
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel { text: qsTr("Attitude Controls") }

            Repeater {
                model: attitudeControlsModel

                RowLayout {
                    Layout.fillWidth: true

                    QGCLabel {
                        Layout.fillWidth: true
                        text: name
                    }

                    Loader {
                        id: channelMonitorLoader
                        width: ScreenTools.defaultFontPixelWidth * 20
                        sourceComponent: attitudeChannelDisplayComponent

                        property var channelMapped: mapped
                        property var channelReversed: reversed
                        property var channelValue: value
                    }
                }
            }
        }

        // Command Buttons
        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCButton {
                id: skipButton
                text: qsTr("Skip")
                onClicked: controller.skipButtonClicked()
            }

            QGCButton {
                id: cancelButton
                text: qsTr("Cancel")
                onClicked: controller.cancelButtonClicked()
            }

            QGCButton {
                id: nextButton
                primary: true
                text: qsTr("Calibrate")

                onClicked: {
                    if (text === qsTr("Calibrate")) {
                        if (controller.channelCount < controller.minChannelCount) {
                            mainWindow.showMessageDialog(qsTr("Radio Not Ready"),
                                                            controller.channelCount == 0 ? qsTr("Please turn on transmitter.") :
                                                                                        (controller.channelCount < controller.minChannelCount ?
                                                                                                qsTr("%1 channels or more are needed to fly.").arg(controller.minChannelCount) :
                                                                                                qsTr("Ready to calibrate.")))
                        } else {
                            mainWindow.showMessageDialog(qsTr("Zero Trims"),
                                                            qsTr("Before calibrating you should zero all your trims and subtrims. Click Ok to start Calibration.\n\n%1").arg(
                                                                (QGroundControl.multiVehicleManager.activeVehicle.px4Firmware ? "" : qsTr("Please ensure all motor power is disconnected AND all props are removed from the vehicle."))),
                                                            Dialog.Ok,
                                                            function() { controller.nextButtonClicked() })
                        }
                    } else {
                        controller.nextButtonClicked()
                    }
                }
            }
        }

        // Status Text
        QGCLabel {
            id: statusText
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width
            wrapMode: Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: qgcPal.text
        }

        Loader {
            id: additionalSetupLoader
            Layout.fillWidth: true
            sourceComponent: additionalSetupComponent
        }
    }

    ColumnLayout {
        id: rightColumnLayout
        Layout.alignment: Qt.AlignTop
        spacing: ScreenTools.defaultFontPixelHeight / 2

        Rectangle {
            id: stickDisplayContainer
            implicitWidth: stickDisplayLayout.width + _margins * 2
            implicitHeight: stickDisplayLayout.height + _margins * 2
            border.color: qgcPal.text
            border.width: 1
            color: qgcPal.window
            radius: ScreenTools.defaultBorderRadius

            property real _margins: ScreenTools.defaultFontPixelHeight / 2
            property real _stickAdjust: leftStickDisplay.width / 2 - _margins * 1.25

            ColumnLayout {
                id: stickDisplayLayout
                anchors.leftMargin: stickDisplayContainer._margins
                anchors.topMargin: stickDisplayContainer._margins
                anchors.left: parent.left
                anchors.top: parent.top
                spacing: stickDisplayContainer._margins

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth * 2

                    QGCComboBox {
                        id: transmitterModeComboBox
                        model: [ qsTr("Mode 1"), qsTr("Mode 2"), qsTr("Mode 3"), qsTr("Mode 4") ]
                        onActivated: (index) => controller.transmitterMode = index + 1

                        Component.onCompleted: currentIndex = controller.transmitterMode - 1
                    }

                    QGCCheckBox {
                        id: centeredThrottleCheckBox
                        text: qsTr("Centered Throttle")
                        checked: controller.centeredThrottle
                        onClicked: controller.centeredThrottle = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: stickDisplayContainer._margins * 2

                    Rectangle {
                        id: leftStickDisplay
                        Layout.alignment: Qt.AlignLeft
                        implicitWidth: ScreenTools.defaultFontPixelHeight * 5
                        implicitHeight: implicitWidth
                        radius: implicitWidth / 2
                        border.color: qgcPal.buttonHighlight
                        border.width: 1
                        color: qgcPal.window

                        Rectangle {
                            x: parent.width / 2 + stickDisplayContainer._stickAdjust * -controller.stickDisplayPositions[0] - width / 2
                            y: parent.height / 2 + stickDisplayContainer._stickAdjust * -controller.stickDisplayPositions[1] - height / 2
                            width: ScreenTools.defaultFontPixelHeight
                            height: width
                            radius: width / 2
                            color: qgcPal.buttonHighlight
                        }
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignRight
                        implicitWidth: leftStickDisplay.implicitWidth
                        implicitHeight: implicitWidth
                        radius: implicitWidth / 2
                        border.color: qgcPal.buttonHighlight
                        border.width: 1
                        color: qgcPal.window

                        Rectangle {
                            x: parent.width / 2 + stickDisplayContainer._stickAdjust * -controller.stickDisplayPositions[2] - width / 2
                            y: parent.height / 2 + stickDisplayContainer._stickAdjust * -controller.stickDisplayPositions[3] - height / 2
                            width: ScreenTools.defaultFontPixelHeight
                            height: width
                            radius: width / 2
                            color: qgcPal.buttonHighlight
                        }
                    }
                }
            }
        }

        RCChannelMonitor {
            Layout.fillWidth: true
            twoColumn: true
        }
    }

    Component {
        id: attitudeChannelDisplayComponent

        Item {
            implicitHeight: ScreenTools.defaultFontPixelHeight

            readonly property int _channelMin: 800
            readonly property int _channelMax: 2200
            readonly property int _channelRange: _channelMax - _channelMin

            property int _lastChannelValue: (_channelMax - _channelMin) / 2 + _channelMin
            property color _barColor: qgcPal.windowShade

            // Bar
            Rectangle {
                id: bar
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: ScreenTools.defaultFontPixelHeight / 2
                color: _barColor
            }

            // Center point
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: globals.defaultTextWidth / 2
                height: bar.height
                color: qgcPal.window
            }

            // Indicator
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.height
                height: width
                radius: width / 2
                color: qgcPal.text
                visible: channelMapped
                x: (((channelReversed ? _channelMax - channelValue : channelValue - _channelMin) / _channelRange) * parent.width) - (width / 2)
            }

            QGCLabel {
                id: notMappedLabel
                anchors.centerIn: parent
                text: qsTr("Not Mapped")
                visible: !channelMapped
            }

            ColorAnimation {
                id: barAnimation
                target: bar
                property: "color"
                from: "yellow"
                to: _barColor
                duration: 1500
            }
        }
    }
}
