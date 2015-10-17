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

import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0

Rectangle {
    color: qgcPal.window

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    Column {
        anchors.fill:   parent
        spacing:        ScreenTools.defaultFontPixelHeight

        QGCLabel {
            width:			parent.width
			wrapMode:		Text.WordWrap
			color:			setupComplete ? qgcPal.text : qgcPal.warningText
            font.pixelSize: ScreenTools.mediumFontPixelSize
			text:           setupComplete ?
                                "Below you will find a summary of the settings for your vehicle. To the left are the setup menus for each component." :
                                "WARNING: Your vehicle requires setup prior to flight. Please resolve the items marked in red using the menu on the left."

            property bool setupComplete: multiVehicleManager.activeVehicle.autopilot.setupComplete
        }

        Flow {
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelWidth

            Repeater {
                model: multiVehicleManager.activeVehicle.autopilot.vehicleComponents


                // Outer summary item rectangle
                Rectangle {
                    width:  ScreenTools.defaultFontPixelWidth * 28
                    height: ScreenTools.defaultFontPixelHeight * 13
                    color:  qgcPal.windowShade

                    readonly property real titleHeight: ScreenTools.defaultFontPixelHeight * 2

                    // Title bar
                    Rectangle {
                        id:     titleBar
                        width:  parent.width
                        height: titleHeight
                        color:  qgcPal.windowShadeDark

                        // Title text
                        QGCLabel {
                            anchors.fill:           parent
                            verticalAlignment:      TextEdit.AlignVCenter
                            horizontalAlignment:    TextEdit.AlignHCenter
                            text:                   modelData.name.toUpperCase()
                        }

                        // Setup indicator
                        Rectangle {
                            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 3
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width:                  10//radius * 2
                            height:                 10//height
                            radius:                 (ScreenTools.defaultFontPixelHeight * .75) * 2
                            color:                  modelData.setupComplete ? "#00d932" : "red"
                            visible:                modelData.requiresSetup
                        }
                    }

                    // Summary Qml
                    Rectangle {
                        anchors.top:    titleBar.bottom
                        width:          parent.width

                        Loader {
                            anchors.fill:   parent
                            source:         modelData.summaryQmlSource
                        }
                    }
                }
            }
        }
    }
}
