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
    id:     _generalRoot
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
        contentWidth:       _generalRoot.width
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior:     Flickable.StopAtBounds

        Column {
            id:                 settingsColumn
            width:              _generalRoot.width
            spacing:            ScreenTools.defaultFontPixelHeight
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   "General Settings"
                font.pixelSize: ScreenTools.mediumFontPixelSize
            }
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.button
            }
            //-----------------------------------------------------------------
            //-- Audio preferences
            QGCCheckBox {
                text:       "Mute all audio output"
                checked:    QGroundControl.isAudioMuted
                onClicked: {
                    QGroundControl.isAudioMuted = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Low power mode
            QGCCheckBox {
                text:       "Enable low power mode"
                checked:    QGroundControl.isLowPowerMode
                onClicked: {
                    QGroundControl.isLowPowerMode = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Prompt Save Log
            QGCCheckBox {
                text:       "Prompt to save Flight Data Log after each flight"
                checked:    QGroundControl.isSaveLogPrompt
                visible:    !ScreenTools.isMobile
                onClicked: {
                    QGroundControl.isSaveLogPrompt = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Clear settings
            QGCCheckBox {
                id:         clearCheck
                text:       "Clear all settings on next start"
                checked:    false
                onClicked: {
                    checked ? clearDialog.visible = true : QGroundControl.clearDeleteAllSettingsNextBoot()
                }
                MessageDialog {
                    id:         clearDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      "Clear Settings"
                    text:       "All saved settings will be reset the next time you start QGroundControl. Is this really what you want?"
                    onYes: {
                        QGroundControl.deleteAllSettingsNextBoot()
                        clearDialog.visible = false
                    }
                    onNo: {
                        clearCheck.checked  = false
                        clearDialog.visible = false
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Map Providers
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    width: ScreenTools.defaultFontPixelWidth * 16
                    text: "Map Providers"
                }
                QGCComboBox {
                    id:     mapProviders
                    width:  ScreenTools.defaultFontPixelWidth * 16
                    model:  QGroundControl.flightMapSettings.mapProviders
                    Component.onCompleted: {
                        var index = mapProviders.find(QGroundControl.flightMapSettings.mapProvider)
                        if (index < 0) {
                            console.warn("Active map provider not in combobox", QGroundControl.flightMapSettings.mapProvider)
                        } else {
                            mapProviders.currentIndex = index
                        }
                    }
                    onActivated: {
                        if (index != -1) {
                            currentIndex = index
                            console.log("New map provider: " + model[index])
                            QGroundControl.flightMapSettings.mapProvider = model[index]
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Palette Styles
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    width: ScreenTools.defaultFontPixelWidth * 16
                    text: "Style"
                }
                QGCComboBox {
                    width: ScreenTools.defaultFontPixelWidth * 16
                    model: [ "Dark", "Light" ]
                    currentIndex: QGroundControl.isDarkStyle ? 0 : 1
                    onActivated: {
                        if (index != -1) {
                            currentIndex = index
                            console.log((index === 0) ? "Now it's Dark" : "Now it's Light")
                            QGroundControl.isDarkStyle = index === 0 ? true : false
                        }
                    }
                }
            }
        }
    }
}
