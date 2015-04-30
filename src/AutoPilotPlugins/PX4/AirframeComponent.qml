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

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0

Rectangle {
    AirframeComponentController { id: controller }
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    color: qgcPal.window

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "AIRFRAME CONFIG"
            font.pointSize: 20
        }

        Item { height: 20; width: 10 } // spacer

        Row {
            width: parent.width

            QGCLabel {
                width:      parent.width - applyButton.width
                text:       "Select you airframe type and specific vehicle bellow. Click 'Apply and Restart' when ready and your vehicle will be disconnected, rebooted to the new settings and re-connected."
                wrapMode:   Text.WordWrap
            }

            QGCButton {
                id:     applyButton
                text:   "Apply and Restart"
                onClicked: { controller.changeAutostart() }
            }
        }

        Item { height: 20; width: 10 } // spacer

        Flow {
            width: parent.width
            spacing: 10

            ExclusiveGroup {
                id: airframeTypeExclusive
            }

            Repeater {
                model: controller.airframeTypes

                // Outer summary item rectangle
                Rectangle {
                    readonly property real titleHeight: 30
                    readonly property real innerMargin: 10

                    width:  250
                    height: 200
                    color:  qgcPal.windowShade

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
                        currentIndex: (modelData.name == controller.currentAirframeType) ? controller.currentVehicleIndex : 0

                        onCurrentIndexChanged: {
                            if (airframeCheckBox.checked) {
                                controller.autostartId = modelData.airframes[currentIndex].autostartId
                            }
                        }
                    }
                }
            }
        }

    }
}
