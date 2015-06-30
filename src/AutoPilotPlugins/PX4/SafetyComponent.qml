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
import QGroundControl.ScreenTools 1.0

QGCView {
    id:         rootQGCView
    viewPanel:  view

    QGCPalette { id: palette; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: rootQGCView }

    property int flightLineWidth: 2             // width of lines for flight graphic
    property int loiterAltitudeColumnWidth: 180 // width of loiter altitude column
    property int shadedMargin: 20               // margin inset for shaded areas
    property int controlVerticalSpacing: 10     // vertical spacing between controls
    property int homeWidth: 50                  // width of home graphic
    property int planeWidth: 40                 // width of plane graphic
    property int arrowToHomeSpacing: 20         // space between down arrow and home graphic
    property int arrowWidth: 18                 // width for arrow graphic
    property int firstColumnWidth: 220          // Width of first column in return home triggers area

    QGCView {
        id: view

        anchors.fill: parent

        Column {
            anchors.fill: parent

            QGCLabel {
                text:           "SAFETY CONFIG"
                font.pixelSize: ScreenTools.largeFontPixelSize
            }

            Item { height: 20; width: 10 } // spacer

            //-----------------------------------------------------------------
            //-- Return Home Triggers

            QGCLabel { text: "Triggers For Return Home"; font.pixelSize: ScreenTools.mediumFontPixelSize; }

            Item { height: 10; width: 10 } // spacer

            Rectangle {
                width: parent.width
                height: triggerColumn.height
                color: palette.windowShade

                Column {
                    id: triggerColumn
                    spacing: controlVerticalSpacing
                    anchors.margins: shadedMargin
                    anchors.left: parent.left

                    // Top margin
                    Item { height: 1; width: 10 }

                    Row {
                        spacing: 10
                        QGCLabel { text: "RC Transmitter Signal Loss"; width: firstColumnWidth; anchors.baseline: rcLossField.baseline }
                        QGCLabel { text: "Return Home after"; anchors.baseline: rcLossField.baseline }
                        FactTextField {
                            id:         rcLossField
                            fact:       controller.getParameterFact(-1, "COM_RC_LOSS_T")
                            showUnits:  true
                        }
                    }

                    Row {
                        spacing: 10
                        FactCheckBox {
                            id:                 telemetryTimeoutCheckbox
                            anchors.baseline:   telemetryLossField.baseline
                            width:              firstColumnWidth
                            fact:               controller.getParameterFact(-1, "COM_DL_LOSS_EN")
                            checkedValue:       1
                            uncheckedValue:     0
                            text:               "Telemetry Signal Timeout"
                        }
                        QGCLabel { text: "Return Home after"; anchors.baseline: telemetryLossField.baseline }
                        FactTextField {
                            id:         telemetryLossField
                            fact:       controller.getParameterFact(-1, "COM_DL_LOSS_T")
                            showUnits:  true
                            enabled:    telemetryTimeoutCheckbox.checked
                        }
                    }

                    // Bottom margin
                    Item { height: 1; width: 10 }
                }
            }

            Item { height: 20; width: 10 }    // spacer

            //-----------------------------------------------------------------
            //-- Return Home Settings

            QGCLabel { text: "Return Home Settings"; font.pixelSize: ScreenTools.mediumFontPixelSize; }

            Item { height: 10; width: 10 } // spacer

            Rectangle {
                width:  parent.width
                height: settingsColumn.height
                color:  palette.windowShade

                Column {
                    id:                 settingsColumn
                    width:              parent.width
                    anchors.margins:    shadedMargin
                    anchors.left:       parent.left

                    Item { height: shadedMargin; width: 10 } // top margin

                    // This item is the holder for the climb alt and loiter seconds fields
                    Item {
                        width:  parent.width
                        height: climbAltitudeColumn.height

                        Column {
                            id:         climbAltitudeColumn
                            spacing:    controlVerticalSpacing

                            QGCLabel { text: "Climb to altitude of" }
                            FactTextField {
                                id:         climbField
                                fact:       controller.getParameterFact(-1, "RTL_RETURN_ALT")
                                showUnits:  true
                            }
                        }


                        Column {
                            x:          flightGraphic.width - 200
                            spacing:    controlVerticalSpacing

                            QGCCheckBox {
                                id:         homeLoiterCheckbox
                                checked:    fact.value > 0
                                text:       "Loiter at Home altitude for"

                                property Fact fact: controller.getParameterFact(-1, "RTL_LAND_DELAY")

                                onClicked: {
                                    fact.value = checked ? 60 : -1
                                }
                            }

                            FactTextField {
                                fact:       controller.getParameterFact(-1, "RTL_LAND_DELAY")
                                showUnits:  true
                                enabled:    homeLoiterCheckbox.checked == true
                            }
                        }
                    }

                    Item { height: 20; width: 10 }    // spacer

                    // This row holds the flight graphic and the home loiter alt column
                    Row {
                        width:      parent.width
                        spacing:    20

                        // Flight graphic
                        Item {
                            id:     flightGraphic
                            width:  parent.width - loiterAltitudeColumnWidth
                            height: 200 // controls the height of the flight graphic

                            Rectangle {
                                x:      planeWidth / 2
                                height: planeImage.y - 5
                                width:  flightLineWidth
                                color:  palette.button
                            }
                            Rectangle {
                                x:      planeWidth / 2
                                height: flightLineWidth
                                width:  parent.width - x
                                color:  palette.button
                            }
                            Rectangle {
                                x:      parent.width - flightLineWidth
                                height: parent.height - homeWidth - arrowToHomeSpacing
                                width:  flightLineWidth
                                color:  palette.button
                            }

                            QGCColoredImage {
                                id:         planeImage
                                y:          parent.height - planeWidth - 40
                                source:     "/qmlimages/SafetyComponentPlane.png"
                                fillMode:   Image.PreserveAspectFit
                                width:      planeWidth
                                height:     planeWidth
                                smooth:     true
                                color:      palette.button
                            }

                            QGCColoredImage {
                                x:          planeWidth + 70
                                y:          parent.height - height - 20
                                width:      80
                                height:     parent.height / 2
                                source:     "/qmlimages/SafetyComponentTree.svg"
                                fillMode:   Image.Stretch
                                smooth:     true
                                color:      palette.windowShadeDark
                            }

                            QGCColoredImage {
                                x:          planeWidth + 15
                                y:          parent.height - height
                                width:      100
                                height:     parent.height * .75
                                source:     "/qmlimages/SafetyComponentTree.svg"
                                fillMode:   Image.PreserveAspectFit
                                smooth:     true
                                color:      palette.button
                            }

                            QGCColoredImage {
                                x:          parent.width - (arrowWidth/2) - 1
                                y:          parent.height - homeWidth - arrowToHomeSpacing - 2
                                source:     "/qmlimages/SafetyComponentArrowDown.png"
                                fillMode:   Image.PreserveAspectFit
                                width:      arrowWidth
                                height:     arrowWidth
                                smooth:     true
                                color:      palette.button
                            }

                            QGCColoredImage {
                                id:         homeImage
                                x:          parent.width - (homeWidth / 2)
                                y:          parent.height - homeWidth
                                source:     "/qmlimages/SafetyComponentHome.png"
                                fillMode:   Image.PreserveAspectFit
                                width:      homeWidth
                                height:     homeWidth
                                smooth:     true
                                color:  palette.button
                            }
                        }

                        Column {
                            spacing: controlVerticalSpacing

                            QGCLabel {
                                text:       "Home loiter altitude";
                                color:      palette.text;
                                enabled:    homeLoiterCheckbox.checked === true
                            }
                            FactTextField {
                                id:         descendField;
                                fact:       controller.getParameterFact(-1, "RTL_DESCEND_ALT")
                                enabled:    homeLoiterCheckbox.checked === true
                                showUnits:  true
                            }
                        }
                    }

                    Item { height: shadedMargin; width: 10 } // bottom margin
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
    } // QGCVIew
}
