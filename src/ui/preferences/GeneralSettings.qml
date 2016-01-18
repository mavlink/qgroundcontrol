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
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:                 _generalRoot
    color:              __qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      settingsColumn.height
        contentWidth:       _generalRoot.width
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:                 settingsColumn
            width:              _generalRoot.width
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                text:   "General Settings"
                font.pixelSize: ScreenTools.mediumFontPixelSize
            }
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.button
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
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
            //-- Prompt Save Log
            QGCCheckBox {
                id:         promptSaveLog
                text:       "Prompt to save Flight Data Log after each flight"
                checked:    QGroundControl.isSaveLogPrompt
                visible:    !ScreenTools.isMobile
                onClicked: {
                    QGroundControl.isSaveLogPrompt = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Prompt Save even if not armed
            QGCCheckBox {
                text:       "Prompt to save Flight Data Log even if vehicle was not armed"
                checked:    QGroundControl.isSaveLogPromptNotArmed
                visible:    !ScreenTools.isMobile
                enabled:    promptSaveLog.checked
                onClicked: {
                    QGroundControl.isSaveLogPromptNotArmed = checked
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

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
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

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }

            //-----------------------------------------------------------------
            //-- Autoconnect settings
            QGCLabel { text: "Autoconnect to the following devices:" }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth * 2

                QGCCheckBox {
                    text:       "Pixhawk"
                    visible:    !ScreenTools.isiOS
                    checked:    QGroundControl.linkManager.autoconnectPixhawk
                    onClicked:  QGroundControl.linkManager.autoconnectPixhawk = checked
                }

                QGCCheckBox {
                    text:       "3DR Radio"
                    visible:    !ScreenTools.isiOS
                    checked:    QGroundControl.linkManager.autoconnect3DRRadio
                    onClicked:  QGroundControl.linkManager.autoconnect3DRRadio = checked
                }

                QGCCheckBox {
                    text:       "PX4 Flow"
                    visible:    !ScreenTools.isiOS
                    checked:    QGroundControl.linkManager.autoconnectPX4Flow
                    onClicked:  QGroundControl.linkManager.autoconnectPX4Flow = checked
                }

                QGCCheckBox {
                    text:       "UDP"
                    checked:    QGroundControl.linkManager.autoconnectUDP
                    onClicked:  QGroundControl.linkManager.autoconnectUDP = checked
                }
            }

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }

            //-----------------------------------------------------------------
            //-- Virtual joystick settings
            QGCCheckBox {
                text:       "Virtual Joystick"
                checked:    QGroundControl.virtualTabletJoystick
                onClicked:  QGroundControl.virtualTabletJoystick = checked
            }

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text:               "Offline mission editing vehicle type:"
                    anchors.baseline:   offlineTypeCombo.baseline
                }

                FactComboBox {
                    id:         offlineTypeCombo
                    width:      ScreenTools.defaultFontPixelWidth * 25
                    fact:       QGroundControl.offlineEditingFirmwareType
                    indexModel: false
                }
            }
        }
    }
}
