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
import QGroundControl.Controllers           1.0

Rectangle {
    id:     _toolbarSettingsRoot
    color:  __qgcPal.window

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    MainToolBarController { id: _controller }

    Flickable {
        clip:               true
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      settingsColumn.height
        contentWidth:       _toolbarSettingsRoot.width
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior:     Flickable.StopAtBounds

        Column {
            id:                 settingsColumn
            width:              _toolbarSettingsRoot.width
            spacing:            ScreenTools.defaultFontPixelHeight
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   "Toolbar Settings"
                font.pixelSize: ScreenTools.mediumFontPixelSize
            }
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.button
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                text:       "Show GPS Indicator"
                checked:    QGroundControl.showGPS
                onClicked: {
                    QGroundControl.showGPS = checked
                }
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                text:       "Show RC Signal Strength"
                checked:    QGroundControl.showRCRSSI
                onClicked: {
                    QGroundControl.showRCRSSI = checked
                }
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                text:       "Show Telemetry Signal Strength"
                checked:    QGroundControl.showTelemetryRSSI
                onClicked: {
                    QGroundControl.showTelemetryRSSI = checked
                }
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                id:         showBattery
                text:       "Show Battery Status"
                checked:    QGroundControl.showBattery
                onClicked: {
                    QGroundControl.showBattery = checked
                }
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                text:       "Show Battery Accumulated Consumption"
                checked:    QGroundControl.showBatteryConsumption
                enabled:    showBattery.checked
                onClicked: {
                    QGroundControl.showBatteryConsumption = checked
                }
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                text:       "Show Flight Mode Selector"
                checked:    QGroundControl.showModeSelector
                onClicked: {
                    QGroundControl.showModeSelector = checked
                }
            }
            //-----------------------------------------------------------------
            QGCCheckBox {
                text:       "Show Arm/Disarm Button"
                checked:    QGroundControl.showArmed
                onClicked: {
                    QGroundControl.showArmed = checked
                }
            }
        }
    }
}
