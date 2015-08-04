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

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

QGCView {
    id:         rootQGCView
    viewPanel:  panel

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    AirframeComponentController {
        id:         controller
        factPanel:  panel

        Component.onCompleted: {
            if (controller.showCustomConfigPanel) {
                showDialog(customConfigDialogComponent, "Custom Airframe Config", 50, StandardButton.Reset)
            }
        }
    }

    Component {
        id: customConfigDialogComponent

        QGCViewMessage {
            id:             customConfigDialog

            message:        "Your vehicle is using a custom airframe configuration. " +
                                "This configuration can only be modified through the Parameter Editor.\n\n" +
                                "If you want to Reset your airframe configuration and select a standard configuration, click 'Reset' above."

            property Fact sys_autostart: controller.getParameterFact(-1, "SYS_AUTOSTART")

            function accept() {
                sys_autostart.value = 0
                customConfigDialog.hideDialog()
            }
        }
    }

    Component {
        id: applyRestartDialogComponent

        QGCViewDialog {
            id: applyRestartDialog

            function accept() {
                controller.changeAutostart()
                applyRestartDialog.hideDialog()
            }

            QGCLabel {
                anchors.fill:   parent
                wrapMode:       Text.WordWrap
                text:           "Clicking Apply will save the changes you have made to your aiframe configuration. " +
                                "Your vehicle will also be rebooted in order to complete the process. " +
                                "After your vehicle reboots, you can reconnect it to QGroundControl."
            }
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCLabel {
            id:             header
            width:          parent.width
            font.pixelSize: ScreenTools.largeFontPixelSize
            text:           "AIRFRAME CONFIG"
        }

        Item {
            id:             headingSpacer
            anchors.top:    header.bottom
            height:         20
            width:          20
        }

        QGCLabel {
            anchors.top:    headingSpacer.bottom
            width:          parent.width - applyButton.width - 5
            text:           "Please select your airframe type. Click 'Apply and Restart' to reboot the autopilot. Please re-connect then manually."
            wrapMode:       Text.WordWrap
        }

        QGCButton {
            id:             applyButton
            anchors.top:    headingSpacer.bottom
            anchors.right:  parent.right
            text:           "Apply and Restart"

            onClicked:      showDialog(applyRestartDialogComponent, "Apply and Restart", 50, StandardButton.Apply | StandardButton.Cancel)
        }

        Item {
            id:             lastSpacer
            anchors.top:    applyButton.bottom
            height:         20
            width:          10
        }

        ScrollView {
            id:                         scroll
            anchors.top:                lastSpacer.bottom
            anchors.bottom:             parent.bottom
            width:                      parent.width
            horizontalScrollBarPolicy:  Qt.ScrollBarAlwaysOff

            Flow {
                width:      scroll.width
                spacing:    10

                ExclusiveGroup {
                    id: airframeTypeExclusive
                }

                Repeater {
                    model: controller.airframeTypes

                    // Outer summary item rectangle
                    Rectangle {
                        id:     airframeBackground
                        readonly property real titleHeight: 30
                        readonly property real innerMargin: 10

                        width:  250
                        height: 200
                        color:  (modelData.name != controller.currentAirframeType) ? qgcPal.windowShade : qgcPal.buttonHighlight

                        MouseArea {
                                anchors.fill: parent
                                onClicked: airframeCheckBox.checked = true
                            }

                        Rectangle {
                            id:     title
                            width:  parent.width
                            height: parent.titleHeight
                            color:  qgcPal.windowShadeDark

                            Text {
                                anchors.fill:   parent

                                color:          qgcPal.buttonText
                                font.pixelSize: 12
                                text:           modelData.name

                                verticalAlignment:      TextEdit.AlignVCenter
                                horizontalAlignment:    TextEdit.AlignHCenter
                            }
                        }

                        Image {
                            id:     image
                            x:      innerMargin
                            width:  parent.width - (innerMargin * 2)
                            height: parent.height - title.height - combo.height - (innerMargin * 3)
                            anchors.topMargin:  innerMargin
                            anchors.top:        title.bottom

                            source:     modelData.imageResource
                            fillMode:   Image.PreserveAspectFit
                            smooth:     true

                        }

                        QGCCheckBox {
                            id:             airframeCheckBox
                            anchors.bottom: image.bottom
                            anchors.right: image.right
                            checked:        modelData.name == controller.currentAirframeType
                            exclusiveGroup: airframeTypeExclusive

                            onCheckedChanged: {
                                if (checked && combo.currentIndex != -1) {
                                    controller.autostartId = modelData.airframes[combo.currentIndex].autostartId
                                    airframeBackground.color = qgcPal.buttonHighlight;
                                } else {
                                    airframeBackground.color = qgcPal.windowShade;
                                }
                            }
                        }

                        QGCComboBox {
                            id:     combo
                            objectName: modelData.airframeType + "ComboBox"
                            x:      innerMargin
                            anchors.topMargin: innerMargin
                            anchors.top: image.bottom
                            width:  parent.width - (innerMargin * 2)
                            model:  modelData.airframes
                            currentIndex: (modelData.name == controller.currentAirframeType) ? controller.currentVehicleIndex : -1

                            onActivated: {
                                if (index != -1) {
                                    currentIndex = index
                                    controller.autostartId = modelData.airframes[index].autostartId
                                    airframeCheckBox.checked = true;
                                }
                            }
                        }
                    }
                } // Repeater - summary boxes
            } // Flow - summary boxes
        } // Scroll View - summary boxes
    } // QGCViewPanel
} // QGCView
