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
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    width: 600
    height: 400

    property var qgcPal: QGCPalette { id: palette; colorGroupEnabled: true }
    property ScreenTools screenTools: ScreenTools { }

    id: topLevel
    objectName: "topLevel"

    color: qgcPal.window

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "VEHICLE SUMMARY"
            font.pointSize: screenTools.dpiAdjustedPointSize(20);
        }

        Item {
            // Just used as a spacer
            height: 15
            width: 10
        }

        QGCLabel {
            width: parent.width
            text: "If any of the setup indicators below are shown as red YOU SHOULD NOT FLY until you complete the setup of those components."
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        Flow {
            width: parent.width
            spacing: 10

            Repeater {
                model: autopilot.components

                // Outer summary item rectangle
                Rectangle {
                    readonly property real titleHeight: 30

                    width:  250
                    height: 200
                    color:  qgcPal.windowShade

                    // Title bar
                    Rectangle {

                        width: parent.width
                        height: titleHeight
                        color: qgcPal.windowShadeDark

                        // Title text
                        Text {
                            anchors.fill:   parent

                            color:          qgcPal.buttonText
                            font.pixelSize: 12
                            text:           modelData.name.toUpperCase()

                            verticalAlignment:      TextEdit.AlignVCenter
                            horizontalAlignment:    TextEdit.AlignHCenter
                        }
                    }

                    // Setup indicator
                    Rectangle {
                        readonly property real indicatorRadius: 6
                        readonly property real indicatorRightInset: 5

                        x:      parent.width - (indicatorRadius * 2) - indicatorRightInset
                        y:      (parent.titleHeight - (indicatorRadius * 2)) / 2
                        width:  indicatorRadius * 2
                        height: indicatorRadius * 2
                        radius: indicatorRadius
                        color:  modelData.setupComplete ? "#00d932" : "red"
                    }

                    // Summary Qml
                    Rectangle {
                        y:      parent.titleHeight
                        width:  parent.width
                        height: parent.height - parent.titleHeight
                        color:  qgcPal.windowShade

                        Loader {
                            anchors.fill: parent
                            source: modelData.summaryQmlSource
                        }
                    }
                }
            }
        }
    }
}
