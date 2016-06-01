/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.5
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
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    property real _minW:        ScreenTools.defaultFontPixelWidth * 30
    property real _boxWidth:    _minW
    property real _boxSpace:    ScreenTools.defaultFontPixelWidth

    function computeDimensions() {
        var sw  = 0
        var rw  = 0
        var idx = Math.floor(scroll.width / (_minW + ScreenTools.defaultFontPixelWidth))
        if(idx < 1) {
            _boxWidth = scroll.width
            _boxSpace = 0
        } else {
            _boxSpace = 0
            if(idx > 1) {
                _boxSpace = ScreenTools.defaultFontPixelWidth
                sw = _boxSpace * (idx - 1)
            }
            rw = scroll.width - sw
            _boxWidth = rw / idx
        }
    }

    AirframeComponentController {
        id:         controller
        factPanel:  panel

        Component.onCompleted: {
            if (controller.showCustomConfigPanel) {
                showDialog(customConfigDialogComponent, qsTr("Custom Airframe Config"), qgcView.showDialogDefaultWidth, StandardButton.Reset)
            }
        }
    }

    Component {
        id: customConfigDialogComponent

        QGCViewMessage {
            id:       customConfigDialog
            message:  qsTr("Your vehicle is using a custom airframe configuration. ") +
                      qsTr("This configuration can only be modified through the Parameter Editor.\n\n") +
                      qsTr("If you want to reset your airframe configuration and select a standard configuration, click 'Reset' above.")

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
                text:           qsTr("Clicking “Apply” will save the changes you have made to your airframe configuration. ") +
                                qsTr("Your vehicle will also be restarted in order to complete the process.")
            }
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        readonly property real spacerHeight: ScreenTools.defaultFontPixelHeight

        Item {
            id:             helpApplyRow
            anchors.top:    parent.top
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         Math.max(helpText.contentHeight, applyButton.height)

            QGCLabel {
                id:             helpText
                width:          parent.width - applyButton.width - 5
                text:           (controller.currentVehicleName != "" ?
                                    qsTr("You've connected a %1.").arg(controller.currentVehicleName) :
                                     qsTr("Airframe is not set.")) +
                                qsTr("To change this configuration, select the desired airframe below then click “Apply and Restart”.")
                font.family:    ScreenTools.demiboldFontFamily
                wrapMode:       Text.WordWrap
            }

            QGCButton {
                id:             applyButton
                anchors.right:  parent.right
                text:           qsTr("Apply and Restart")

                onClicked:      showDialog(applyRestartDialogComponent, qsTr("Apply and Restart"), qgcView.showDialogDefaultWidth, StandardButton.Apply | StandardButton.Cancel)
            }
        }

        Item {
            id:             lastSpacer
            anchors.top:    helpApplyRow.bottom
            height:         parent.spacerHeight
            width:          10
        }

        QGCFlickable {
            id:             scroll
            anchors.top:    lastSpacer.bottom
            anchors.bottom: parent.bottom
            width:          parent.width
            clip:           true
            contentHeight:  flowView.height
            contentWidth:   parent.width
            flickableDirection: Flickable.VerticalFlick

            onWidthChanged: {
                computeDimensions()
            }

            Flow {
                id:         flowView
                width:      scroll.width
                spacing:    _boxSpace

                ExclusiveGroup {
                    id: airframeTypeExclusive
                }

                Repeater {
                    model: controller.airframeTypes

                    // Outer summary item rectangle
                    Rectangle {
                        width:  _boxWidth
                        height: ScreenTools.defaultFontPixelHeight * 14
                        color:  qgcPal.window

                        readonly property real titleHeight: ScreenTools.defaultFontPixelHeight * 1.75
                        readonly property real innerMargin: ScreenTools.defaultFontPixelWidth

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                applyButton.primary = true
                                airframeCheckBox.checked = true
                            }
                        }

                        QGCLabel {
                            id:     title
                            text:   modelData.name
                        }

                        Rectangle {
                            anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
                            anchors.top:        title.bottom
                            anchors.bottom:     parent.bottom
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            color:              airframeCheckBox.checked ? qgcPal.buttonHighlight : qgcPal.windowShade

                            Image {
                                id:                 image
                                anchors.margins:    innerMargin
                                anchors.top:        parent.top
                                anchors.bottom:     combo.top
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                fillMode:           Image.PreserveAspectFit
                                smooth:             true
                                mipmap:             true
                                source:             modelData.imageResource
                            }

                            QGCCheckBox {
                                // Although this item is invisible we still use it to manage state
                                id:             airframeCheckBox
                                checked:        modelData.name == controller.currentAirframeType
                                exclusiveGroup: airframeTypeExclusive
                                visible:        false

                                onCheckedChanged: {
                                    if (checked && combo.currentIndex != -1) {
                                        controller.autostartId = modelData.airframes[combo.currentIndex].autostartId
                                    }
                                }
                            }

                            QGCComboBox {
                                id:                 combo
                                objectName:         modelData.airframeType + "ComboBox"
                                anchors.margins:    innerMargin
                                anchors.bottom:     parent.bottom
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                model:              modelData.airframes

                                Component.onCompleted: {
                                    if (airframeCheckBox.checked) {
                                        currentIndex = controller.currentVehicleIndex
                                    }
                                }

                                onActivated: {
                                    applyButton.primary = true
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
