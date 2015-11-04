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
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0


QGCView {
    id:                 _safetyView
    viewPanel:          panel
    anchors.fill:       parent

    FactPanelController { id: controller; factPanel: panel }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

    property real firstColumnWidth:     ScreenTools.defaultFontPixelWidth * 28
    property real secondColumnWidth:    ScreenTools.defaultFontPixelWidth * 25

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Flickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      safetyColumn.height
            contentWidth:       parent.width
            boundsBehavior:     Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            Column {
                id:     safetyColumn
                width:  _safetyView.width

                //-----------------------------------------------------------------
                //-- Return Home Triggers

                QGCLabel { text: "Triggers For Return Home"; font.pixelSize: ScreenTools.mediumFontPixelSize; }

                Item { height: ScreenTools.defaultFontPixelHeight * 0.5; width: 1 } // spacer

                Rectangle {
                    width:  parent.width
                    height: triggerColumn.height + ScreenTools.defaultFontPixelHeight
                    color:  palette.windowShade
                    Column {
                        id:                 triggerColumn
                        width:              parent.width
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        anchors.left:       parent.left
                        Item { height: ScreenTools.defaultFontPixelHeight * 0.5; width: 1 } // spacer
                        Row {
                            spacing:                ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                text:               "RC Transmitter Signal Loss"
                                width:              firstColumnWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCLabel {
                                text:               "Return Home after"
                                width:              secondColumnWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            FactTextField {
                                id:                 rcLossField
                                fact:               controller.getParameterFact(-1, "COM_RC_LOSS_T")
                                showUnits:          true
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Row {
                            spacing:                ScreenTools.defaultFontPixelWidth
                            FactCheckBox {
                                id:                 telemetryTimeoutCheckbox
                                width:              firstColumnWidth
                                fact:               controller.getParameterFact(-1, "COM_DL_LOSS_EN")
                                checkedValue:       1
                                uncheckedValue:     0
                                text:               "Telemetry Signal Timeout"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCLabel {
                                text:               "Return Home after"
                                width:              secondColumnWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            FactTextField {
                                id:                 telemetryLossField
                                fact:               controller.getParameterFact(-1, "COM_DL_LOSS_T")
                                showUnits:          true
                                enabled:            telemetryTimeoutCheckbox.checked
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Item { height: ScreenTools.defaultFontPixelHeight * 0.5; width: 1 } // spacer
                    }
                }

                Item { height: ScreenTools.defaultFontPixelHeight; width: 1 } // spacer

                //-----------------------------------------------------------------
                //-- Return Home Settings

                QGCLabel { text: "Return Home Settings"; font.pixelSize: ScreenTools.mediumFontPixelSize; }

                Item { height: ScreenTools.defaultFontPixelHeight * 0.5; width: 1 } // spacer

                Rectangle {
                    width:  parent.width
                    height: settingsRow.height
                    color:  palette.windowShade

                    Row {
                        id:                 settingsRow
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        anchors.left:       parent.left
                        spacing:            ScreenTools.defaultFontPixelWidth
                        Item {
                            width:          firstColumnWidth
                            height:         firstColumnWidth * 0.65
                            Image {
                                id:             icon
                                width:          parent.width
                                height:         parent.width * 0.5
                                mipmap:         true
                                fillMode:       Image.PreserveAspectFit
                                visible:        false
                                source:         "/qmlimages/ReturnToHomeAltitude.svg"
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            ColorOverlay {
                                id:             iconOverlay
                                anchors.fill:   icon
                                source:         icon
                                color:          palette.button
                            }
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Column {
                            width:              parent.width - firstColumnWidth
                            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.verticalCenter: parent.verticalCenter
                            Item { height: ScreenTools.defaultFontPixelHeight * 0.5; width: 1 } // spacer
                            Row {
                                spacing:        ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       "Climb to altitude of"
                                    width:      secondColumnWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                FactTextField {
                                    id:         climbField
                                    fact:       controller.getParameterFact(-1, "RTL_RETURN_ALT")
                                    showUnits:  true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Row {
                                spacing:        ScreenTools.defaultFontPixelWidth
                                QGCCheckBox {
                                    id:         homeLoiterCheckbox
                                    width:      secondColumnWidth
                                    checked:    fact.value > 0
                                    text:       "Loiter at Home altitude for"
                                    anchors.verticalCenter: parent.verticalCenter
                                    property Fact fact: controller.getParameterFact(-1, "RTL_LAND_DELAY")
                                    onClicked: {
                                        fact.value = checked ? 60 : -1
                                    }
                                }
                                FactTextField {
                                    id:         landDelayField
                                    fact:       controller.getParameterFact(-1, "RTL_LAND_DELAY")
                                    showUnits:  true
                                    enabled:    homeLoiterCheckbox.checked === true
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                            }
                            Row {
                                spacing:    ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       "Home loiter altitude";
                                    color:      palette.text;
                                    enabled:    homeLoiterCheckbox.checked === true
                                    width:      secondColumnWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                FactTextField {
                                    id:         descendField;
                                    fact:       controller.getParameterFact(-1, "RTL_DESCEND_ALT")
                                    enabled:    homeLoiterCheckbox.checked === true
                                    showUnits:  true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Item { height: ScreenTools.defaultFontPixelHeight; width: 1 } // spacer
                        }
                    }
                }

                QGCLabel {
                    width:          parent.width
                    font.pixelSize: ScreenTools.mediumFontPixelSize
                    text:           "Warning: You have an advanced safety configuration set using the NAV_RCL_OBC parameter. The above settings may not apply.";
                    visible:        fact.value !== 0
                    wrapMode:       Text.Wrap

                    property Fact fact: controller.getParameterFact(-1, "NAV_RCL_OBC")
                }

                QGCLabel {
                    width:          parent.width
                    font.pixelSize: ScreenTools.mediumFontPixelSize
                    text:           "Warning: You have an advanced safety configuration set using the NAV_DLL_OBC parameter. The above settings may not apply.";
                    visible:        fact.value !== 0
                    wrapMode:       Text.Wrap

                    property Fact fact: controller.getParameterFact(-1, "NAV_DLL_OBC")
                }
            }
        }
    }
}
