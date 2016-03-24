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

import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    property real _margins: ScreenTools.defaultFontPixelWidth
    property Fact _frame:   controller.getParameterFact(-1, "FRAME")

    APMAirframeComponentController {
        id:         controller
        factPanel:  panel
    }

    ExclusiveGroup {
        id: airframeTypeExclusive
    }

    Component {
        id: applyRestartDialogComponent

        QGCViewDialog {
            id: applyRestartDialog

            Connections {
                target: controller
                onCurrentAirframeTypeChanged: {
                    airframePicker.model = controller.currentAirframeType.airframes;
                }
                onCurrentAirframeChanged : {
                    hideDialog();
                }
            }

            QGCLabel {
                id:                 applyParamsText
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.margins:    _margins
                wrapMode:           Text.WordWrap
                text:               qsTr("Select you drone to load the default parameters for it. ")
            }

            Flow {
                anchors.margins:    _margins
                anchors.top:        applyParamsText.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                spacing :           _margins
                layoutDirection:    Qt.Vertical;

                Repeater {
                    id:     airframePicker
                    model:  controller.currentAirframeType.airframes;

                    delegate: QGCButton {
                        id:     btnParams
                        width:  parent.width / 2.1
                        height: (ScreenTools.defaultFontPixelHeight * 14) / 5
                        text:   controller.currentAirframeType.airframes[index].name;

                        onClicked : controller.currentAirframe = controller.currentAirframeType.airframes[index]
                    }
                }
            }
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Item {
            id:             helpApplyRow
            anchors.top:    parent.top
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         Math.max(helpText.contentHeight, applyButton.height)

            QGCLabel {
                id:                     helpText
                anchors.rightMargin:    _margins
                anchors.left:           parent.left
                anchors.right:          applyButton.right
                text:                   qsTr("Please select your airframe type")
                font.pixelSize:         ScreenTools.mediumFontPixelSize
                wrapMode:               Text.WordWrap
            }

            QGCButton {
                id:             applyButton
                anchors.right:  parent.right
                text:           qsTr("Load common parameters")
                onClicked:      showDialog(applyRestartDialogComponent, qsTr("Load common parameters"), qgcView.showDialogDefaultWidth, StandardButton.Close)
            }
        }

        Item {
            id:             helpSpacer
            anchors.top:    helpApplyRow.bottom
            height:         parent.spacerHeight
            width:          10
        }

        QGCFlickable {
            id:             scroll
            anchors.top:    helpSpacer.bottom
            anchors.bottom: parent.bottom
            anchors.left:   parent.left
            anchors.right:  parent.right
            contentHeight:  frameColumn.height
            contentWidth:   frameColumn.width



            Column {
                id:         frameColumn
                spacing:    _margins

                Repeater {
                    model: controller.airframeTypesModel

                    QGCRadioButton {
                        text: object.name
                        checked: controller.currentAirframeType == object
                        exclusiveGroup: airframeTypeExclusive

                        onCheckedChanged: {
                            if (checked) {
                                controller.currentAirframeType = object
                            }
                        }
                    }
                }
            }
        }
    } // QGCViewPanel
} // QGCView
