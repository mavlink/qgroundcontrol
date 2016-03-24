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
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce: QGroundControl.multiVehicleManager.disconnectedVehicle.battery.percentRemainingAnnounce

    QGCPalette { id: qgcPal }

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      settingsColumn.height
        contentWidth:       settingsColumn.width

        Column {
            id:                 settingsColumn
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            spacing:            ScreenTools.defaultFontPixelHeight / 2

            QGCLabel {
                text:   qsTr("General Settings")
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
            //-- Units

            Row {
                spacing:    ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    id:                 distanceUnitsLabel
                    anchors.baseline:   distanceUnitsCombo.baseline
                    text:               qsTr("Distance units:")
                }

                FactComboBox {
                    id:         distanceUnitsCombo
                    width:      ScreenTools.defaultFontPixelWidth * 10
                    fact:       QGroundControl.distanceUnits
                    indexModel: false
                }

                QGCLabel {
                    anchors.baseline:   distanceUnitsCombo.baseline
                    text:               qsTr("(requires reboot to take affect)")
                }

            }

            Row {
                spacing:    ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    anchors.baseline:   speedUnitsCombo.baseline
                    width:              distanceUnitsLabel.width
                    text:               qsTr("Speed units:")
                }

                FactComboBox {
                    id:         speedUnitsCombo
                    width:      ScreenTools.defaultFontPixelWidth * 20
                    fact:       QGroundControl.speedUnits
                    indexModel: false
                }

                QGCLabel {
                    anchors.baseline:   speedUnitsCombo.baseline
                    text:              qsTr("(requires reboot to take affect)")
                }
            }

            //-----------------------------------------------------------------
            //-- Audio preferences
            QGCCheckBox {
                text:       qsTr("Mute all audio output")
                checked:    QGroundControl.isAudioMuted
                onClicked: {
                    QGroundControl.isAudioMuted = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Prompt Save Log
            QGCCheckBox {
                id:         promptSaveLog
                text:       qsTr("Prompt to save Flight Data Log after each flight")
                checked:    QGroundControl.isSaveLogPrompt
                visible:    !ScreenTools.isMobile
                onClicked: {
                    QGroundControl.isSaveLogPrompt = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Prompt Save even if not armed
            QGCCheckBox {
                text:       qsTr("Prompt to save Flight Data Log even if vehicle was not armed")
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
                text:       qsTr("Clear all settings on next start")
                checked:    false
                onClicked: {
                    checked ? clearDialog.visible = true : QGroundControl.clearDeleteAllSettingsNextBoot()
                }
                MessageDialog {
                    id:         clearDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      qsTr("Clear Settings")
                    text:       qsTr("All saved settings will be reset the next time you start QGroundControl. Is this really what you want?")
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
            //-- Battery talker
            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCCheckBox {
                    id:                 announcePercentCheckbox
                    anchors.baseline:   announcePercent.baseline
                    text:               qsTr("Announce battery percent lower than:")
                    checked:            _percentRemainingAnnounce.value != 0

                    onClicked: {
                        if (checked) {
                            _percentRemainingAnnounce.value = _percentRemainingAnnounce.defaultValueString
                        } else {
                            _percentRemainingAnnounce.value = 0
                        }
                    }
                }

                FactTextField {
                    id:         announcePercent
                    fact:       _percentRemainingAnnounce
                    enabled:    announcePercentCheckbox.checked
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
                    anchors.baseline:   mapProviders.baseline
                    width:              ScreenTools.defaultFontPixelWidth * 16
                    text:               qsTr("Map Providers:")
                }
                QGCComboBox {
                    id:     mapProviders
                    width:  ScreenTools.defaultFontPixelWidth * 16
                    model:  QGroundControl.flightMapSettings.mapProviders
                    Component.onCompleted: {
                        var index = mapProviders.find(QGroundControl.flightMapSettings.mapProvider)
                        if (index < 0) {
                            console.warn(qsTr("Active map provider not in combobox"), QGroundControl.flightMapSettings.mapProvider)
                        } else {
                            mapProviders.currentIndex = index
                        }
                    }
                    onActivated: {
                        if (index != -1) {
                            currentIndex = index
                            console.log(qsTr("New map provider: ") + model[index])
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
                    anchors.baseline:   paletteCombo.baseline
                    width:              ScreenTools.defaultFontPixelWidth * 16
                    text:               qsTr("Style:")
                }
                QGCComboBox {
                    id: paletteCombo
                    width: ScreenTools.defaultFontPixelWidth * 16
                    model: [ qsTr("Indoor"), qsTr("Outdoor") ]
                    currentIndex: QGroundControl.isDarkStyle ? 0 : 1
                    onActivated: {
                        if (index != -1) {
                            currentIndex = index
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
                    text:       qsTr("Pixhawk")
                    visible:    !ScreenTools.isiOS
                    checked:    QGroundControl.linkManager.autoconnectPixhawk
                    onClicked:  QGroundControl.linkManager.autoconnectPixhawk = checked
                }

                QGCCheckBox {
                    text:       qsTr("SiK Radio")
                    visible:    !ScreenTools.isiOS
                    checked:    QGroundControl.linkManager.autoconnect3DRRadio
                    onClicked:  QGroundControl.linkManager.autoconnect3DRRadio = checked
                }

                QGCCheckBox {
                    text:       qsTr("PX4 Flow")
                    visible:    !ScreenTools.isiOS
                    checked:    QGroundControl.linkManager.autoconnectPX4Flow
                    onClicked:  QGroundControl.linkManager.autoconnectPX4Flow = checked
                }

                QGCCheckBox {
                    text:       qsTr("UDP")
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
                text:       qsTr("Virtual Joystick")
                checked:    QGroundControl.virtualTabletJoystick
                onClicked:  QGroundControl.virtualTabletJoystick = checked
            }

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }

            //-----------------------------------------------------------------
            //-- Offline mission editing settings
            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text:               qsTr("Offline mission editing vehicle type:")
                    anchors.baseline:   offlineTypeCombo.baseline
                }

                FactComboBox {
                    id:         offlineTypeCombo
                    width:      ScreenTools.defaultFontPixelWidth * 25
                    fact:       QGroundControl.offlineEditingFirmwareType
                    indexModel: false
                }
            }

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }

            //-----------------------------------------------------------------
            //-- Experimental Survey settings
            QGCCheckBox {
                text:       qsTr("Experimental Survey [WIP - no bugs reports please]")
                checked:    QGroundControl.experimentalSurvey
                onClicked:  QGroundControl.experimentalSurvey = checked
            }
        }
    }
}
