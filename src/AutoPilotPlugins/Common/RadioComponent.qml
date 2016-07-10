/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    readonly property string    dialogTitle:            qsTr("Radio")
    readonly property real      labelToMonitorMargin:   defaultTextWidth * 3

    property bool controllerCompleted:      false
    property bool controllerAndViewReady:   false

    function updateChannelCount()
    {
/*
            FIXME: Turned off for now, since it prevents binding. Need to restructure to
            allow binding and still check channel count
            if (controller.channelCount < controller.minChannelCount) {
                showDialog(channelCountDialogComponent, dialogTitle, qgcView.showDialogDefaultWidth, 0)
            } else {
                hideDialog()
            }
*/
    }

    RadioComponentController {
        id:             controller
        factPanel:      panel
        statusText:     statusText
        cancelButton:   cancelButton
        nextButton:     nextButton
        skipButton:     skipButton

        Component.onCompleted: {
            controllerCompleted = true
            if (qgcView.completedSignalled) {
                controllerAndViewReady = true
                controller.start()
                updateChannelCount()
            }
        }

        onChannelCountChanged:              updateChannelCount()
        onFunctionMappingChangedAPMReboot:  showMessage(qsTr("Reboot required"), qsTr("Your stick mappings have changed, you must reboot the vehicle for correct operation."), StandardButton.Ok)
        onThrottleReversedCalFailure:       showMessage(qsTr("Throttle channel reversed"), qsTr("Calibration failed. The throttle channel on your transmitter is reversed. You must correct this on your transmitter in order to complete calibration."), StandardButton.Ok)
    }

    onCompleted: {
        if (controllerCompleted) {
            controllerAndViewReady = true
            controller.start()
            updateChannelCount()
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Component {
            id: copyTrimsDialogComponent

            QGCViewMessage {
                message: qsTr("Center your sticks and move throttle all the way down, then press Ok to copy trims. After pressing Ok, reset the trims on your radio back to zero.")

                function accept() {
                    hideDialog()
                    controller.copyTrims()
                }
            }
        }

        Component {
            id: zeroTrimsDialogComponent

            QGCViewMessage {
                message: qsTr("Before calibrating you should zero all your trims and subtrims. Click Ok to start Calibration.\n\n%1").arg(
                         (QGroundControl.multiVehicleManager.activeVehicle.px4Firmware ? "" : qsTr("Please ensure all motor power is disconnected AND all props are removed from the vehicle.")))

                function accept() {
                    hideDialog()
                    controller.nextButtonClicked()
                }
            }
        }

        Component {
            id: channelCountDialogComponent

            QGCViewMessage {
                message: controller.channelCount == 0 ? qsTr("Please turn on transmitter.") : qsTr("%1 channels or more are needed to fly.").arg(controller.minChannelCount)
            }
        }

        Component {
            id: spektrumBindDialogComponent

            QGCViewDialog {

                function accept() {
                    controller.spektrumBindMode(radioGroup.current.bindMode)
                    hideDialog()
                }

                function reject() {
                    hideDialog()
                }

                Column {
                    anchors.fill:   parent
                    spacing:        5

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       qsTr("Click Ok to place your Spektrum receiver in the bind mode. Select the specific receiver type below:")
                    }

                    ExclusiveGroup { id: radioGroup }

                    QGCRadioButton {
                        exclusiveGroup: radioGroup
                        text:           qsTr("DSM2 Mode")

                        property int bindMode: RadioComponentController.DSM2
                    }

                    QGCRadioButton {
                        exclusiveGroup: radioGroup
                        text:           qsTr("DSMX (7 channels or less)")

                        property int bindMode: RadioComponentController.DSMX7
                    }

                    QGCRadioButton {
                        exclusiveGroup: radioGroup
                        checked:        true
                        text:           qsTr("DSMX (8 channels or more)")

                        property int bindMode: RadioComponentController.DSMX8
                    }
                }
            }
        } // Component - spektrumBindDialogComponent

        // Live channel monitor control component
        Component {
            id: channelMonitorDisplayComponent

            Item {
                property int    rcValue:    1500


                property int            __lastRcValue:      1500
                readonly property int   __rcValueMaxJitter: 2
                property color          __barColor:         qgcPal.windowShade

                readonly property int _pwmMin:      800
                readonly property int _pwmMax:      2200
                readonly property int _pwmRange:    _pwmMax - _pwmMin

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
                    radius:                 width / 2
                    color:                  qgcPal.text
                    visible:                mapped
                    x:                      (((reversed ? _pwmMax - rcValue : rcValue - _pwmMin) / _pwmRange) * parent.width) - (width / 2)
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
                // FIXME: Bar animation is turned off for now to figure out better usbaility
                onRcValueChanged: {
                    if (Math.abs(rcValue - __lastRcValue) > __rcValueMaxJitter) {
                        __lastRcValue = rcValue
                        barAnimation.restart()
                    }
                }

                // rcValue debugger
                QGCLabel {
                    anchors.fill: parent
                    text: rcValue
                }
                */
            }
        } // Component - channelMonitorDisplayComponent

        // Main view Qml starts here

        QGCFlickable {
            anchors.fill:   parent
            contentHeight:  Math.max(leftColumn.height, rightColumn.height)
            clip:           true

            // Left side column
            Column {
                id:             leftColumn
                anchors.left:   parent.left
                anchors.right:  columnSpacer.left
                spacing:        10

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
                            height:             qgcView.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

                            property real defaultTextWidth: qgcView.defaultTextWidth
                            property bool mapped:           controller.rollChannelMapped
                            property bool reversed:         controller.rollChannelReversed
                        }

                        Connections {
                            target: controller

                            onRollChannelRCValueChanged: rollLoader.item.rcValue = rcValue
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
                            height:             qgcView.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

                            property real defaultTextWidth: qgcView.defaultTextWidth
                            property bool mapped:           controller.pitchChannelMapped
                            property bool reversed:         controller.pitchChannelReversed
                        }

                        Connections {
                            target: controller

                            onPitchChannelRCValueChanged: pitchLoader.item.rcValue = rcValue
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
                            height:             qgcView.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

                            property real defaultTextWidth: qgcView.defaultTextWidth
                            property bool mapped:           controller.yawChannelMapped
                            property bool reversed:         controller.yawChannelReversed
                        }

                        Connections {
                            target: controller

                            onYawChannelRCValueChanged: yawLoader.item.rcValue = rcValue
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
                            height:             qgcView.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

                            property real defaultTextWidth: qgcView.defaultTextWidth
                            property bool mapped:           controller.throttleChannelMapped
                            property bool reversed:         controller.throttleChannelReversed
                        }

                        Connections {
                            target: controller

                            onThrottleChannelRCValueChanged: throttleLoader.item.rcValue = rcValue
                        }
                    }
                } // Column - Attitude Control labels

                // Command Buttons
                Row {
                    spacing: 10

                    QGCButton {
                        id:         skipButton
                        text:       qsTr("Skip")

                        onClicked: controller.skipButtonClicked()
                    }

                    QGCButton {
                        id:         cancelButton
                        text:       qsTr("Cancel")

                        onClicked: controller.cancelButtonClicked()
                    }

                    QGCButton {
                        id:         nextButton
                        primary:    true
                        text:       qsTr("Calibrate")

                        onClicked: {
                            if (text == qsTr("Calibrate")) {
                                showDialog(zeroTrimsDialogComponent, dialogTitle, qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                            } else {
                                controller.nextButtonClicked()
                            }
                        }
                    }
                } // Row - Buttons

                // Status Text
                QGCLabel {
                    id:         statusText
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                }

                Item {
                    width: 10
                    height: defaultTextHeight * 4
                }

                Rectangle {
                    width:          parent.width
                    height:         1
                    border.color:   qgcPal.text
                    border.width:   1
                }

                QGCLabel { text: qsTr("Additional Radio setup:") }

                QGCButton {
                    id:         bindButton
                    text:       qsTr("Spektrum Bind")

                    onClicked: showDialog(spektrumBindDialogComponent, dialogTitle, qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                }

                QGCButton {
                    text:       qsTr("Copy Trims")
                    visible:    QGroundControl.multiVehicleManager.activeVehicle.px4Firmware
                    onClicked:  showDialog(copyTrimsDialogComponent, dialogTitle, qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                }

                Repeater {
                    model: QGroundControl.multiVehicleManager.activeVehicle.px4Firmware ? [ "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3"] : 0

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth
                        property Fact fact: controller.getParameterFact(-1, modelData)

                        QGCLabel {
                            anchors.baseline:   optCombo.baseline
                            text:               fact.shortDescription + ":"
                        }

                        FactComboBox {
                            id:         optCombo
                            width:      ScreenTools.defaultFontPixelWidth * 15
                            fact:       parent.fact
                            indexModel: false
                        }
                    }
                } // Repeater
            } // Column - Left Column

            Item {
                id:             columnSpacer
                anchors.right:  rightColumn.left
                width:          20
            }

            // Right side column
            Column {
                id:             rightColumn
                anchors.top:    parent.top
                anchors.right:  parent.right
                width:          Math.min(defaultTextWidth * 35, qgcView.width * 0.4)
                spacing:        ScreenTools.defaultFontPixelHeight / 2

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    ExclusiveGroup { id: modeGroup }

                    QGCRadioButton {
                        exclusiveGroup: modeGroup
                        text:           qsTr("Mode 1")
                        checked:        controller.transmitterMode == 1

                        onClicked: controller.transmitterMode = 1
                    }

                    QGCRadioButton {
                        exclusiveGroup: modeGroup
                        text:           qsTr("Mode 2")
                        checked:        controller.transmitterMode == 2

                        onClicked: controller.transmitterMode = 2
                    }
                }

                Image {
                    width:      parent.width
                    fillMode:   Image.PreserveAspectFit
                    smooth:     true
                    source:     controller.imageHelp
                }

                RCChannelMonitor {
                    width: parent.width
                }
            } // Column - Right Column
        } // QGCFlickable
    } // QGCViewPanel
}
