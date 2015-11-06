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

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:     __mavlinkRoot
    color:  __qgcPal.window

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    Flickable {
        clip:               true
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      settingsColumn.height
        contentWidth:       __mavlinkRoot.width
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior:     Flickable.StopAtBounds

        Column {
            id:                 settingsColumn
            width:              __mavlinkRoot.width
            spacing:            ScreenTools.defaultFontPixelHeight
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   "MavLink Settings"
                font.pixelSize: ScreenTools.mediumFontPixelSize
            }
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.button
            }
            //-----------------------------------------------------------------
            //-- Mavlink Heartbeats
            QGCCheckBox {
                text:       "Emit heartbeat"
                checked:    QGroundControl.isHeartBeatEnabled
                onClicked: {
                    QGroundControl.isHeartBeatEnabled = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Multiplexing
            QGCCheckBox {
                text:       "Enable multiplexing (forward packets to all other links)"
                checked:    QGroundControl.isMultiplexingEnabled
                onClicked: {
                    QGroundControl.isMultiplexingEnabled = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Version Check
            QGCCheckBox {
                text:       "Only accept MAVs with same protocol version"
                checked:    QGroundControl.isVersionCheckEnabled
                onClicked: {
                    QGroundControl.isVersionCheckEnabled = checked
                }
            }
        }
    }
}
