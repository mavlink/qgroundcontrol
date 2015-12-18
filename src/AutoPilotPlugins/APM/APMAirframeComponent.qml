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
    property Fact sysIdFact:    controller.getParameterFact(-1, "FRAME")

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

    APMAirframeComponentController {
        id:         controller
        factPanel:  panel
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
                id: applyParamsText
                anchors.top:   parent.top
                anchors.left:  parent.left
                anchors.right: parent.right
                anchors.margins: _boxSpace
                wrapMode:       Text.WordWrap
                text:           "Select you drone to load the default parameters for it. "
            }

            Flow {
                anchors.top : applyParamsText.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing : _boxSpace
                layoutDirection: Qt.Vertical;
                anchors.margins : _boxSpace
                Repeater {
                    id : airframePicker
                    model : controller.currentAirframeType.airframes;

                    delegate: QGCButton {
                        id: btnParams
                        width:  parent.width / 2.1
                        height: (ScreenTools.defaultFontPixelHeight * 14) / 5
                        text:                   controller.currentAirframeType.airframes[index].name;

                        onClicked : {
                            controller.currentAirframe = controller.currentAirframeType.airframes[index]
                        }
                    }
                }
            }
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        readonly property real spacerHeight: ScreenTools.defaultFontPixelHeight

        onWidthChanged: {
            computeDimensions()
        }

        Item {
            id:             helpApplyRow
            anchors.top:    parent.top
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         Math.max(helpText.contentHeight, applyButton.height)

            QGCLabel {
                id:             helpText
                width:          parent.width - applyButton.width - 5
                text:           qsTr("Please select your airframe type")
                font.pixelSize: ScreenTools.mediumFontPixelSize
                wrapMode:       Text.WordWrap
            }

            QGCButton {
                id:             applyButton
                anchors.right:  parent.right
                text:           qsTr("Load common parameters")

                onClicked:      showDialog(applyRestartDialogComponent, qsTr("Load common parameters"), 50, StandardButton.Close)
            }
        }

        Item {
            id:             lastSpacer
            anchors.top:    helpApplyRow.bottom
            height:         parent.spacerHeight
            width:          10
        }

        Flickable {
            id:             scroll
            anchors.top:    lastSpacer.bottom
            width:          parent.width;
            height:         parent.height;
            clip:           true
            boundsBehavior:     Flickable.StopAtBounds
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
                    model: controller.airframeTypesModel

                    // Outer summary item rectangle
                    delegate : Rectangle {
                        id:     airframeBackground
                        width:  _boxWidth
                        height: ScreenTools.defaultFontPixelHeight * 14
                        color:  qgcPal.windowShade;

                        readonly property real titleHeight: ScreenTools.defaultFontPixelHeight * 1.75
                        readonly property real innerMargin: ScreenTools.defaultFontPixelWidth

                        MouseArea {
                                anchors.fill:   parent
                                onClicked:      airframeCheckBox.checked = true;
                        }

                        Rectangle {
                            id:     nameRect;
                            width:  parent.width
                            height: parent.titleHeight
                            color:  qgcPal.windowShadeDark

                            QGCLabel {
                                anchors.fill:           parent
                                color:                  qgcPal.buttonText
                                verticalAlignment:      TextEdit.AlignVCenter
                                horizontalAlignment:    TextEdit.AlignHCenter
                                text:                   object.name
                            }
                        }

                        Image {
                            id:                 imageRect
                            anchors.topMargin:  innerMargin
                            anchors.top:        nameRect.bottom
                            width:              parent.width * 0.75
                            height:             parent.height - nameRect.height - (innerMargin * 3)
                            fillMode:           Image.PreserveAspectFit
                            smooth:             true
                            mipmap:             true
                            source:             object.imageResource
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCCheckBox {
                            id:             airframeCheckBox
                            checked:        object.type == sysIdFact.value
                            exclusiveGroup: airframeTypeExclusive
                            anchors.bottom: imageRect.bottom
                            anchors.right:  parent.right
                            anchors.rightMargin: innerMargin

                            onCheckedChanged: {
                                if (checked) {
                                    controller.currentAirframeType = object
                                }
                                airframeBackground.color = checked ? qgcPal.buttonHighlight : qgcPal.windowShade;
                            }
                        }
                    }
                }
            }
        } // Scroll View - summary boxes
    } // QGCViewPanel
} // QGCView
