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
    id:             _summaryRoot
    anchors.fill:   parent
    color:          qgcPal.window

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    function capitalizeWords(sentence) {
        return sentence.replace(/(?:^|\s)\S/g, function(a) { return a.toUpperCase(); });
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Flickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      summaryFlow.y + summaryFlow.height
        flickableDirection: Flickable.VerticalFlick

        QGCLabel {
            id:             topLabel
            width:			parent.width
            wrapMode:		Text.WordWrap
            color:			setupComplete ? qgcPal.text : qgcPal.warningText
            font.weight:    setupComplete ? Font.Normal : Font.DemiBold
            text:           setupComplete ?
                                "Below you will find a summary of the settings for your vehicle. To the left are the setup menus for each component." :
                                "WARNING: Your vehicle requires setup prior to flight. Please resolve the items marked in red using the menu on the left."
            property bool setupComplete: multiVehicleManager.activeVehicle.autopilot.setupComplete
        }

        Flow {
            id:                 summaryFlow
            anchors.topMargin:  _margins
            anchors.top:        topLabel.bottom
            width:              parent.width
            spacing:            ScreenTools.defaultFontPixelWidth

            Repeater {
                model: multiVehicleManager.activeVehicle.autopilot.vehicleComponents

                QGCLabel {
                    width:          summaryRectangle.width
                    height:         summaryRectangle.y + summaryRectangle.height
                    text:           capitalizeWords(modelData.name)
                    font.weight:    Font.DemiBold
                    visible:        modelData.summaryQmlSource.toString() != ""
                    color:          modelData.setupComplete ? qgcPal.text : "red"

                    Rectangle {
                        id:     summaryRectangle
                        y:      parent.contentHeight + (_margins / 2)
                        width:  summaryLoader.width + _margins
                        height: summaryLoader.height + _margins
                        color:  qgcPal.windowShade

                        Loader {
                            id:                 summaryLoader
                            anchors.margins:    _margins / 2
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            source:             modelData.summaryQmlSource
                        }
                    }
                }
            }
        }
    }
}
