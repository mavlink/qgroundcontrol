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
import QtQuick.Layouts  1.11

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0

SetupPage {
    id:             radioPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width:  availableWidth
            height: Math.max(leftColumn.height, rightColumn.height)

            readonly property string  dialogTitle: qsTr("Radio")

            function setupPageCompleted() {
                controller.start()
                updateChannelCount()
            }

            function updateChannelCount()
            {
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: radioPage.enabled }

            RadioComponentController {
                id:             controller
                statusText:     statusText
                cancelButton:   cancelButton
                nextButton:     nextButton
                skipButton:     skipButton
                onChannelCountChanged:              updateChannelCount()
                onFunctionMappingChangedAPMReboot:  mainWindow.showMessageDialog(qsTr("Reboot required"), qsTr("Your stick mappings have changed, you must reboot the vehicle for correct operation."))
                onThrottleReversedCalFailure:       mainWindow.showMessageDialog(qsTr("Throttle channel reversed"), qsTr("Calibration failed. The throttle channel on your transmitter is reversed. You must correct this on your transmitter in order to complete calibration."))
            }

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
                        controller.spektrumBindMode(radioGroup.checkedButton.bindMode)
                        hideDialog()
                    }

                    function reject() {
                        hideDialog()
                    }

                    ButtonGroup { id: radioGroup }

                    Column {
                        anchors.fill:   parent
                        spacing:        5

                        QGCLabel {
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            text:       qsTr("Click Ok to place your Spektrum receiver in the bind mode. Select the specific receiver type below:")
                        }

                        QGCRadioButton {
                            text:               qsTr("DSM2 Mode")
                            ButtonGroup.group:  radioGroup
                            property int bindMode: RadioComponentController.DSM2
                        }

                        QGCRadioButton {
                            text:               qsTr("DSMX (7 channels or less)")
                            ButtonGroup.group:  radioGroup
                            property int bindMode: RadioComponentController.DSMX7
                        }

                        QGCRadioButton {
                            checked:            true
                            text:               qsTr("DSMX (8 channels or more)")
                            ButtonGroup.group:  radioGroup
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
                        width:                      globals.defaultTextWidth / 2
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
                }
            } // Component - channelMonitorDisplayComponent

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
                        height: globals.defaultTextHeight * 2
                        QGCLabel {
                            id:     rollLabel
                            width:  globals.defaultTextWidth * 10
                            text:   qsTr("Roll")
                        }

                        Loader {
                            id:                 rollLoader
                            anchors.left:       rollLabel.right
                            anchors.right:      parent.right
                            height:             globals.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

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
                        height: globals.defaultTextHeight * 2

                        QGCLabel {
                            id:     pitchLabel
                            width:  globals.defaultTextWidth * 10
                            text:   qsTr("Pitch")
                        }

                        Loader {
                            id:                 pitchLoader
                            anchors.left:       pitchLabel.right
                            anchors.right:      parent.right
                            height:             globals.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

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
                        height: globals.defaultTextHeight * 2

                        QGCLabel {
                            id:     yawLabel
                            width:  globals.defaultTextWidth * 10
                            text:   qsTr("Yaw")
                        }

                        Loader {
                            id:                 yawLoader
                            anchors.left:       yawLabel.right
                            anchors.right:      parent.right
                            height:             globals.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

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
                        height: globals.defaultTextHeight * 2

                        QGCLabel {
                            id:     throttleLabel
                            width:  globals.defaultTextWidth * 10
                            text:   qsTr("Throttle")
                        }

                        Loader {
                            id:                 throttleLoader
                            anchors.left:       throttleLabel.right
                            anchors.right:      parent.right
                            height:             globals.defaultTextHeight
                            width:              100
                            sourceComponent:    channelMonitorDisplayComponent

                            property bool mapped:           controller.throttleChannelMapped
                            property bool reversed:         controller.throttleChannelReversed
                        }

                        Connections {
                            target:                             controller
                            onThrottleChannelRCValueChanged:    throttleLoader.item.rcValue = rcValue
                        }
                    }
                } // Column - Attitude Control labels

                // Command Buttons
                Row {
                    spacing: 10

                    QGCButton {
                        id:         skipButton
                        text:       qsTr("Skip")
                        onClicked:  controller.skipButtonClicked()
                    }

                    QGCButton {
                        id:         cancelButton
                        text:       qsTr("Cancel")
                        onClicked:  controller.cancelButtonClicked()
                    }

                    QGCButton {
                        id:         nextButton
                        primary:    true
                        text:       qsTr("Calibrate")

                        onClicked: {
                            if (text === qsTr("Calibrate")) {
                                mainWindow.showComponentDialog(zeroTrimsDialogComponent, dialogTitle, mainWindow.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
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

                Rectangle {
                    width:          parent.width
                    height:         1
                    border.color:   qgcPal.text
                    border.width:   1
                }

                QGCLabel { text: qsTr("Additional Radio setup:") }

                GridLayout {
                    id:                 switchSettingsGrid
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    columns:            2
                    columnSpacing:      ScreenTools.defaultFontPixelWidth

                    Repeater {
                        model: QGroundControl.multiVehicleManager.activeVehicle.px4Firmware ?
                                   (QGroundControl.multiVehicleManager.activeVehicle.multiRotor ?
                                       [ "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3"] :
                                       [ "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2", "RC_MAP_PARAM1", "RC_MAP_PARAM2", "RC_MAP_PARAM3"]) :
                                   0

                        RowLayout {
                            Layout.fillWidth: true

                            property Fact fact: controller.getParameterFact(-1, modelData)

                            QGCLabel {
                                Layout.fillWidth:   true
                                text:               fact.shortDescription
                            }
                            FactComboBox {
                                width:      ScreenTools.defaultFontPixelWidth * 15
                                fact:       parent.fact
                                indexModel: false
                            }
                        }
                    }
                }

                RowLayout {
                    QGCButton {
                        id:         bindButton
                        text:       qsTr("Spektrum Bind")
                        onClicked:  mainWindow.showComponentDialog(spektrumBindDialogComponent, dialogTitle, mainWindow.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                    }

                    QGCButton {
                        text:       qsTr("Copy Trims")
                        onClicked:  mainWindow.showComponentDialog(copyTrimsDialogComponent, dialogTitle, mainWindow.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                    }
                }
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
                width:          ScreenTools.defaultFontPixelWidth * 40
                spacing:        ScreenTools.defaultFontPixelHeight / 2

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCRadioButton {
                        text:       qsTr("Mode 1")
                        checked:    controller.transmitterMode == 1
                        onClicked:  controller.transmitterMode = 1
                    }

                    QGCRadioButton {
                        text:       qsTr("Mode 2")
                        checked:    controller.transmitterMode == 2
                        onClicked:  controller.transmitterMode = 2
                    }
                }

                Image {
                    width:      parent.width
                    fillMode:   Image.PreserveAspectFit
                    smooth:     true
                    source:     controller.imageHelp
                }

                RCChannelMonitor {
                    width:      parent.width
                    twoColumn:  true
                }
            } // Column - Right Column
        } // Item
    } // Component - pageComponent
} // SetupPage
