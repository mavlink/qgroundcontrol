/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

SetupPage {
    id:             airframePage
    pageComponent:  (controller && controller.showCustomConfigPanel) ? customFrame : pageComponent

    AirframeComponentController {
        id:         controller
    }

    Component {
        id: customFrame
        Column {
            width:          availableWidth
            spacing:        ScreenTools.defaultFontPixelHeight * 4
            Item {
                width:      1
                height:     1
            }
            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                width:      parent.width * 0.5
                height:     ScreenTools.defaultFontPixelHeight * 4
                wrapMode:   Text.WordWrap
                text:       qsTr("Your vehicle is using a custom airframe configuration. ") +
                            qsTr("This configuration can only be modified through the Parameter Editor.\n\n") +
                            qsTr("If you want to reset your airframe configuration and select a standard configuration, click 'Reset' below.")
            }
            QGCButton {
                text:       qsTr("Reset")
                enabled:    sys_autostart
                anchors.horizontalCenter: parent.horizontalCenter
                property Fact sys_autostart: controller.getParameterFact(-1, "SYS_AUTOSTART")
                onClicked: {
                    if(sys_autostart) {
                        sys_autostart.value = 0
                    }
                }
            }
        }
    }

    Component {
        id: pageComponent

        Column {
            id:     mainColumn
            width:  availableWidth

            property real _minW:        ScreenTools.defaultFontPixelWidth * 30
            property real _boxWidth:    _minW
            property real _boxSpace:    ScreenTools.defaultFontPixelWidth

            readonly property real spacerHeight: ScreenTools.defaultFontPixelHeight

            onWidthChanged: {
                computeDimensions()
            }

            Component.onCompleted: computeDimensions()

            function computeDimensions() {
                var sw  = 0
                var rw  = 0
                var idx = Math.floor(mainColumn.width / (_minW + ScreenTools.defaultFontPixelWidth))
                if(idx < 1) {
                    _boxWidth = mainColumn.width
                    _boxSpace = 0
                } else {
                    _boxSpace = 0
                    if(idx > 1) {
                        _boxSpace = ScreenTools.defaultFontPixelWidth
                        sw = _boxSpace * (idx - 1)
                    }
                    rw = mainColumn.width - sw
                    _boxWidth = rw / idx
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
                        text:           qsTr("Clicking “Apply” will save the changes you have made to your airframe configuration.<br><br>\
All vehicle parameters other than Radio Calibration will be reset.<br><br>\
Your vehicle will also be restarted in order to complete the process.")
                    }
                }
            }

            Item {
                id:             helpApplyRow
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

                    onClicked:      mainWindow.showComponentDialog(applyRestartDialogComponent, qsTr("Apply and Restart"), mainWindow.showDialogDefaultWidth, StandardButton.Apply | StandardButton.Cancel)
                }
            }

            Item {
                id:             lastSpacer
                height:         parent.spacerHeight
                width:          10
            }

            Flow {
                id:         flowView
                width:      parent.width
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
                                checked:        modelData.name === controller.currentAirframeType
                                exclusiveGroup: airframeTypeExclusive
                                visible:        false

                                onCheckedChanged: {
                                    if (checked && combo.currentIndex !== -1) {
                                        console.log("check box change", combo.currentIndex)
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
                                textRole:           "text"

                                Component.onCompleted: {
                                    if (airframeCheckBox.checked) {
                                        currentIndex = controller.currentVehicleIndex
                                    }
                                }

                                onActivated: {
                                    applyButton.primary = true
                                    airframeCheckBox.checked = true;
                                    console.log("combo change", index)
                                    controller.autostartId = modelData.airframes[index].autostartId
                                }
                            }
                        }
                    }
                } // Repeater - summary boxes
            } // Flow - summary boxes
        } // Column
    } // Component
} // SetupPage
