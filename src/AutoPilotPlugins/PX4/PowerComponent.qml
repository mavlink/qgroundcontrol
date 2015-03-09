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

/// @file
///     @brief Battery, propeller and magnetometer settings
///     @author Gus Grubba <mavlink@grubba.com>

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    QGCPalette { id: palette; colorGroupEnabled: true }

    width:  600
    height: 600
    color:  palette.window

    property int firstColumnWidth: 220
    property ScreenTools __screenTools: ScreenTools { }

    Column {
        anchors.fill: parent
        spacing: 10

        QGCLabel {
            text: "POWER CONFIG"
            font.pointSize: 20 * __screenTools.dpiFactor;
        }

        Item { height: 1; width: 10 }

        QGCLabel {
            text: "Battery"
            color: palette.text
            font.pointSize: 20 * __screenTools.dpiFactor;
        }

        Rectangle {
            width: parent.width
            height: 160
            color: palette.windowShade

            Column {
                id: batteryColumn
                spacing: 10
                anchors.verticalCenter: parent.verticalCenter
                x: (parent.x + 20)

                Row {
                    spacing: 10
                    QGCLabel { text: "Number of Cells"; width: firstColumnWidth; anchors.baseline: cellsField.baseline}
                    FactTextField {
                        id: cellsField
                        fact: Fact { name: "BAT_N_CELLS" }
                        showUnits: true
                    }
                }

                Row {
                    spacing: 10
                    QGCLabel { text: "Full Voltage (per cell)"; width: firstColumnWidth; anchors.baseline: battHighField.baseline}
                    FactTextField {
                        id: battHighField
                        fact: Fact { name: "BAT_V_CHARGED" }
                        showUnits: true
                    }
                }

                Row {
                    spacing: 10
                    QGCLabel { text: "Empty Voltage (per cell)"; width: firstColumnWidth; anchors.baseline: battLowField.baseline}
                    FactTextField {
                        id: battLowField
                        fact: Fact { name: "BAT_V_EMPTY" }
                        showUnits: true
                    }
                }

                Row {
                    spacing: 10
                    QGCLabel { text: "Voltage Drop on Full Load (per cell)"; width: firstColumnWidth; anchors.baseline: battDropField.baseline}
                    FactTextField {
                        id: battDropField
                        fact: Fact { name: "BAT_V_LOAD_DROP" }
                        showUnits: true
                    }
                }
            }
        }
    }
}
