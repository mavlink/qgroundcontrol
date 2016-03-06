/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.1

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

/// PX4 Advanced Flight Mode configuration
Item {
    id: root

    // The following properties must be pushed in from the Loader
    //property var qgcView      - QGCView control
    //property var qgcViewPanel - QGCViewPanel control

    readonly property int monitorThresholdCharWidth: 8  // Character width of Monitor and Threshold labels

    // User visible strings

    readonly property string title:                     "FLIGHT MODES"


    property string topHelpText:                        "Assign Flight Modes to radio control channels and adjust the thresholds for triggering them. " +
                                                        "You can assign multiple flight modes to a single channel. " +
                                                        "Turn your radio control on to test switch settings. " +
                                                        "The following channels: " + controller.reservedChannels +
                                                        " are not available for Flight Modes since they are already in use for other functions."


    readonly property string fwManualModeName:          "Manual/Main"
    readonly property string mrManualModeName:          "Stabilized/Main"
    readonly property string fwManualModeDescription:   "The pilot has full control of the aircraft, no assistance is provided. " +
                                                        "The Main mode switch must always be assigned to a channel in order to fly"
    readonly property string mrManualModeDescription:   "The pilot has full control of the aircraft, only attitude is stabilized. " +
                                                        "The Main mode switch must always be assigned to a channel in order to fly"

    readonly property string assistModeName:            "Assist"
    readonly property string assistModeDescription:     "If Position Control is placed on a seperate channel from the Main mode channel, an additional 'Assist' mode is added to the Main switch. " +
                                                        "In order for the Attitude Control/Position Control switch to be active, the Main switch must be in Assist mode."

    readonly property string autoModeName:              "Auto"
    readonly property string autoModeDescription:       "If Loiter is placed on a seperate channel from the Main mode channel, an additional 'Auto' mode is added to the Main switch. " +
                                                        "In order for the Mission/Loiter switch to be active, the Main switch must be in Auto mode."

    readonly property string fwAcroModeName:            "Stabilized"
    readonly property string mrAcroModeName:            "Acro"
    readonly property string fwAcroModeDescription:     "The angular rates are controlled, but not the attitude. "
    readonly property string mrAcroModeDescription:     "The angular rates are controlled, but not the attitude. "

    readonly property string altCtlModeName:            "Altitude Control"
    readonly property string fwAltCtlModeDescription:   "Roll stick controls banking, pitch stick altitude " +
                                                        "Throttle stick controls speed. " +
                                                        "With no stick inputs the plane holds heading, but drifts off in wind. "
    readonly property string mrAltCtlModeDescription:   "Same as Stablized mode except that Throttle controls climb/sink rate. Centered Throttle holds altitude steady. "

    readonly property string posCtlModeName:            "Position Control"
    readonly property string fwPosCtlModeDescription:   "Roll stick controls banking, pitch stick controls altitude. " +
                                                        "Throttle stick controls speed." +
                                                        "With no stick inputs the plane flies a straight line, even in wind. "
    readonly property string mrPosCtlModeDescription:   "Roll and Pitch sticks control sideways and forward speed " +
                                                        "Throttle stick controls climb / sink rade. "

    readonly property string missionModeName:           "Auto Mission"
    readonly property string missionModeDescription:    "The aircraft obeys the programmed mission sent by QGroundControl. "

    readonly property string loiterModeName:            "Auto Pause"
    readonly property string fwLoiterModeDescription:   "The aircraft flies in a circle around the current position at the current altitude. "
    readonly property string mrLoiterModeDescription:   "The multirotor hovers at the current position and altitude. "

    readonly property string returnModeName:            "Return"
    readonly property string returnModeDescription:     "The vehicle returns to the home position, loiters and then lands. "

    readonly property string offboardModeName:          "Offboard"
    readonly property string offboardModeDescription:   "All flight control aspects are controlled by an offboard system."

    readonly property real modeSpacing: ScreenTools.defaultFontPixelHeight / 3

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    PX4AdvancedFlightModesController {
        id:         controller
        factPanel:  qgcViewPanel

        onModeRowsChanged: recalcModePositions()
    }

    Timer {
        interval:   200
        running:    true

        onTriggered: {
            recalcModePositions()
        }
    }

    function recalcModePositions() {
        var spacing = ScreenTools.defaultFontPixelHeight / 2
        var nextY = manualMode.y + manualMode.height + spacing

        for (var index = 0; index < 9; index++) {
            if (controller.assistModeRow == index) {
                if (controller.assistModeVisible) {
                    assistMode.y = nextY
                    assistMode.z = 9 - index
                    nextY += assistMode.height + spacing
                }
            } else if (controller.autoModeRow == index) {
                if (controller.autoModeVisible) {
                    autoMode.y = nextY
                    autoMode.z = 9 - index
                    nextY += autoMode.height  + spacing
                }
            } else if (controller.acroModeRow == index) {
                acroMode.y = nextY
                acroMode.z = 9 - index
                nextY += acroMode.height + spacing
            } else if (controller.altCtlModeRow == index) {
                altCtlMode.y = nextY
                altCtlMode.z = 9 - index
                nextY += altCtlMode.height + spacing
            } else if (controller.posCtlModeRow == index) {
                posCtlMode.y = nextY
                posCtlMode.z = 9 - index
                nextY += posCtlMode.height + spacing
            } else if (controller.loiterModeRow == index) {
                loiterMode.y = nextY
                loiterMode.z = 9 - index
                nextY += loiterMode.height + spacing
            } else if (controller.missionModeRow == index) {
                missionMode.y = nextY
                missionMode.z = 9 - index
                nextY += missionMode.height + spacing
            } else if (controller.returnModeRow == index) {
                returnMode.y = nextY
                returnMode.z = 9 - index
                nextY += returnMode.height + spacing
            } else if (controller.offboardModeRow == index) {
                offboardMode.y = nextY
                offboardMode.z = 9 - index
                nextY += offboardMode.height + spacing
            }
        }

        scrollItem.height = nextY
    }

    Component {
        id: joystickEnabledDialogComponent

        QGCViewMessage {
            message: "Flight Mode Config is disabled since you have a Joystick enabled."
        }
    }

    ScrollView {
        id:                         scroll
        anchors.fill:               parent
        horizontalScrollBarPolicy:  Qt.ScrollBarAlwaysOff

        Item {
            id:     scrollItem
            width:  scroll.viewport.width

            Item {
                id:             helpApplyRow
                width:          parent.width
                height:         Math.max(helpText.contentHeight, applyButton.y + applyButton.height)

                QGCLabel {
                    id:                     helpText
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.left:           parent.left
                    anchors.right:          buttonColumn.left
                    text:                   topHelpText
                    font.pixelSize:         ScreenTools.defaultFontPixelSize
                    wrapMode:               Text.WordWrap
                }

                Column {
                    id:                     buttonColumn
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.right:          parent.right
                    spacing:                ScreenTools.defaultFontPixelHeight / 4

                    QGCButton {
                        text: "Use Single Channel Mode Selection"
                        visible: controller.parameterExists(-1, "RC_MAP_FLTMODE")
                        onClicked: {
                            controller.getParameterFact(-1, "RC_MAP_MODE_SW").value = 0
                            controller.getParameterFact(-1, "RC_MAP_FLTMODE").value = 5
                        }
                    }

                    QGCButton {
                        id:                     applyButton
                        text:                   "Generate Thresholds"
                        onClicked: controller.generateThresholds()
                    }
                }
            }

            Item {
                id:             lastSpacer
                anchors.top:    helpApplyRow.bottom
                height:         ScreenTools.defaultFontPixelHeight
                width:          10
            }

            ModeSwitchDisplay {
                id:                     manualMode
                anchors.top:            lastSpacer.bottom
                flightModeName:         controller.fixedWing ? fwManualModeName : mrManualModeName
                flightModeDescription:  controller.fixedWing ? fwManualModeDescription : mrManualModeDescription
                rcValue:                controller.manualModeRcValue
                modeChannelIndex:       controller.manualModeChannelIndex
                modeChannelEnabled:     true
                modeSelected:           controller.manualModeSelected
                thresholdValue:         controller.manualModeThreshold
                thresholdDragEnabled:   false

                onModeChannelIndexSelected: controller.manualModeChannelIndex = index
            }

            ModeSwitchDisplay {
                id:                     assistMode
                visible:                controller.assistModeVisible
                flightModeName:         assistModeName
                flightModeDescription:  assistModeDescription
                rcValue:                controller.assistModeRcValue
                modeChannelIndex:       controller.assistModeChannelIndex
                modeChannelEnabled:     false
                modeSelected:           controller.assistModeSelected
                thresholdValue:         controller.assistModeThreshold
                thresholdDragEnabled:   true

                onThresholdValueChanged: controller.assistModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     autoMode
                visible:                controller.autoModeVisible
                flightModeName:         autoModeName
                flightModeDescription:  autoModeDescription
                rcValue:                controller.autoModeRcValue
                modeChannelIndex:       controller.autoModeChannelIndex
                modeChannelEnabled:     false
                modeSelected:           controller.autoModeSelected
                thresholdValue:         controller.autoModeThreshold
                thresholdDragEnabled:   true

                onThresholdValueChanged: controller.autoModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     acroMode
                flightModeName:         controller.fixedWing ? fwAcroModeName : mrAcroModeName
                flightModeDescription:  controller.fixedWing ? fwAcroModeDescription : mrAcroModeDescription
                rcValue:                controller.acroModeRcValue
                modeChannelIndex:       controller.acroModeChannelIndex
                modeChannelEnabled:     true
                modeSelected:           controller.acroModeSelected
                thresholdValue:         controller.acroModeThreshold
                thresholdDragEnabled:   true

                onModeChannelIndexSelected:  controller.acroModeChannelIndex = index
                onThresholdValueChanged:    controller.acroModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     altCtlMode
                flightModeName:         altCtlModeName
                flightModeDescription:  controller.fixedWing ? fwAltCtlModeDescription : mrAltCtlModeDescription
                rcValue:                controller.altCtlModeRcValue
                modeChannelIndex:       controller.altCtlModeChannelIndex
                modeChannelEnabled:     false
                modeSelected:           controller.altCtlModeSelected
                thresholdValue:         controller.altCtlModeThreshold
                thresholdDragEnabled:   !controller.assistModeVisible

                onThresholdValueChanged:    controller.altCtlModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     posCtlMode
                flightModeName:         posCtlModeName
                flightModeDescription:  controller.fixedWing ? fwPosCtlModeDescription : mrPosCtlModeDescription
                rcValue:                controller.posCtlModeRcValue
                modeChannelIndex:       controller.posCtlModeChannelIndex
                modeChannelEnabled:     true
                modeSelected:           controller.posCtlModeSelected
                thresholdValue:         controller.posCtlModeThreshold
                thresholdDragEnabled:   true

                onModeChannelIndexSelected:  controller.posCtlModeChannelIndex = index
                onThresholdValueChanged:    controller.posCtlModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     missionMode
                flightModeName:         missionModeName
                flightModeDescription:  missionModeDescription
                rcValue:                controller.missionModeRcValue
                modeChannelIndex:       controller.missionModeChannelIndex
                modeChannelEnabled:     false
                modeSelected:           controller.missionModeSelected
                thresholdValue:         controller.missionModeThreshold
                thresholdDragEnabled:   !controller.autoModeVisible

                onThresholdValueChanged: controller.missionModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     loiterMode
                flightModeName:         loiterModeName
                flightModeDescription:  controller.fixedWing ? fwLoiterModeDescription : mrLoiterModeDescription
                rcValue:                controller.loiterModeRcValue
                modeChannelIndex:       controller.loiterModeChannelIndex
                modeChannelEnabled:     true
                modeSelected:           controller.loiterModeSelected
                thresholdValue:         controller.loiterModeThreshold
                thresholdDragEnabled:   true

                onModeChannelIndexSelected:  controller.loiterModeChannelIndex = index
                onThresholdValueChanged:    controller.loiterModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     returnMode
                flightModeName:         returnModeName
                flightModeDescription:  returnModeDescription
                rcValue:                controller.returnModeRcValue
                modeChannelIndex:       controller.returnModeChannelIndex
                modeChannelEnabled:     true
                modeSelected:           controller.returnModeSelected
                thresholdValue:         controller.returnModeThreshold
                thresholdDragEnabled:   true

                onModeChannelIndexSelected:  controller.returnModeChannelIndex = index
                onThresholdValueChanged:    controller.returnModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }

            ModeSwitchDisplay {
                id:                     offboardMode
                flightModeName:         offboardModeName
                flightModeDescription:  offboardModeDescription
                rcValue:                controller.offboardModeRcValue
                modeChannelIndex:       controller.offboardModeChannelIndex
                modeChannelEnabled:     true
                modeSelected:           controller.offboardModeSelected
                thresholdValue:         controller.offboardModeThreshold
                thresholdDragEnabled:   true

                onModeChannelIndexSelected:  controller.offboardModeChannelIndex = index
                onThresholdValueChanged:    controller.offboardModeThreshold = thresholdValue

                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 1000 } }
            }
        } // Item
    } // Scroll View
} // Item
