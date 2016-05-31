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
            }

            QGCLabel {
                id:                 applyParamsText
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.margins:    _margins
                wrapMode:           Text.WordWrap
                text:               qsTr("Select your drone to load the default parameters for it. ")
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

                        onClicked : {
                            controller.loadParameters(controller.currentAirframeType.airframes[index].params)
                            hideDialog()
                        }
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
                font.pointSize:         ScreenTools.mediumFontPointSize
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
