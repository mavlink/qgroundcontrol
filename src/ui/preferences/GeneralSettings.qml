/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce:    QGroundControl.batteryPercentRemainingAnnounce
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 15

    QGCPalette { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      settingsColumn.height
            contentWidth:       settingsColumn.width

            Column {
                id:                 settingsColumn
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                spacing:            ScreenTools.defaultFontPixelHeight / 2

                //-----------------------------------------------------------------
                //-- Base UI Font Point Size
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        id:     baseFontLabel
                        text:   qsTr("Base UI font size:")
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Row {
                        id:         baseFontRow
                        spacing:    ScreenTools.defaultFontPixelWidth / 2
                        anchors.verticalCenter: parent.verticalCenter

                        QGCButton {
                            id:     decrementButton
                            width:  height
                            height: baseFontEdit.height
                            text:   "-"

                            onClicked: {
                                if(ScreenTools.defaultFontPointSize > 6) {
                                    QGroundControl.baseFontPointSize = QGroundControl.baseFontPointSize - 1
                                }
                            }
                        }

                        QGCTextField {
                            id:             baseFontEdit
                            width:          _editFieldWidth - (decrementButton.width * 2) - (baseFontRow.spacing * 2)
                            text:           QGroundControl.baseFontPointSize
                            showUnits:      true
                            unitsLabel:     "pt"
                            maximumLength:  6
                            validator:      DoubleValidator {bottom: 6.0; top: 48.0; decimals: 2;}

                            onEditingFinished: {
                                var point = parseFloat(text)
                                if(point >= 6.0 && point <= 48.0)
                                    QGroundControl.baseFontPointSize = point;
                            }
                        }

                        QGCButton {
                            width:  height
                            height: baseFontEdit.height
                            text:   "+"

                            onClicked: {
                                if(ScreenTools.defaultFontPointSize < 49) {
                                    QGroundControl.baseFontPointSize = QGroundControl.baseFontPointSize + 1
                                }
                            }
                        }
                    }

                    QGCLabel {
                        anchors.verticalCenter: parent.verticalCenter
                        text:                   qsTr("(requires app restart)")
                    }
                }

                //-----------------------------------------------------------------
                //-- Units

                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        width:              baseFontLabel.width
                        anchors.baseline:   distanceUnitsCombo.baseline
                        text:               qsTr("Distance units:")
                    }

                    FactComboBox {
                        id:                 distanceUnitsCombo
                        width:              _editFieldWidth
                        fact:               QGroundControl.distanceUnits
                        indexModel:         false
                    }

                    QGCLabel {
                        anchors.baseline:   distanceUnitsCombo.baseline
                        text:               qsTr("(requires app restart)")
                    }

                }

                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        width:              baseFontLabel.width
                        anchors.baseline:   areaUnitsCombo.baseline
                        text:               qsTr("Area units:")
                    }

                    FactComboBox {
                        id:                 areaUnitsCombo
                        width:              _editFieldWidth
                        fact:               QGroundControl.areaUnits
                        indexModel:         false
                    }

                    QGCLabel {
                        anchors.baseline:   areaUnitsCombo.baseline
                        text:               qsTr("(requires app restart)")
                    }

                }

                Row {
                    spacing:                ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        width:              baseFontLabel.width
                        anchors.baseline:   speedUnitsCombo.baseline
                        text:               qsTr("Speed units:")
                    }

                    FactComboBox {
                        id:                 speedUnitsCombo
                        width:              _editFieldWidth
                        fact:               QGroundControl.speedUnits
                        indexModel:         false
                    }

                    QGCLabel {
                        anchors.baseline:   speedUnitsCombo.baseline
                        text:               qsTr("(requires app restart)")
                    }
                }

                Item {
                    height: ScreenTools.defaultFontPixelHeight / 2
                    width:  parent.width
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
                        text:               qsTr("Announce battery lower than:")
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
                        id:                 announcePercent
                        fact:               _percentRemainingAnnounce
                        enabled:            announcePercentCheckbox.checked
                    }
                }

                Item {
                    height: ScreenTools.defaultFontPixelHeight / 2
                    width:  parent.width
                }

                //-----------------------------------------------------------------
                //-- Map Providers
                Row {

                    /*
                      TODO: Map settings should come from QGroundControl.mapEngineManager. What is currently in
                      QGroundControl.flightMapSettings should be moved there so all map related funtions are in
                      one place.
                     */

                    spacing:    ScreenTools.defaultFontPixelWidth
                    visible:    QGroundControl.flightMapSettings.googleMapEnabled

                    QGCLabel {
                        id:                 mapProvidersLabel
                        anchors.baseline:   mapProviders.baseline
                        text:               qsTr("Map Provider:")
                    }

                    QGCComboBox {
                        id:                 mapProviders
                        width:              _editFieldWidth
                        model:              QGroundControl.flightMapSettings.mapProviders
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
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        width:              mapProvidersLabel.width
                        anchors.baseline:   paletteCombo.baseline
                        text:               qsTr("Style:")
                    }

                    QGCComboBox {
                        id:             paletteCombo
                        width:          _editFieldWidth
                        model:          [ qsTr("Indoor"), qsTr("Outdoor") ]
                        currentIndex:   QGroundControl.isDarkStyle ? 0 : 1

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

                    QGCCheckBox {
                        text:       qsTr("RTK GPS")
                        checked:    QGroundControl.linkManager.autoconnectRTKGPS
                        onClicked:  QGroundControl.linkManager.autoconnectRTKGPS = checked
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

                QGCLabel { text: "Offline mission editing" }

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:               qsTr("Firmware:")
                        width:              hoverSpeedLabel.width
                        anchors.baseline:   offlineTypeCombo.baseline
                    }

                    FactComboBox {
                        id:         offlineTypeCombo
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        fact:       QGroundControl.offlineEditingFirmwareType
                        indexModel: false
                    }
                }

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:               qsTr("Vehicle:")
                        width:              hoverSpeedLabel.width
                        anchors.baseline:   offlineVehicleCombo.baseline
                    }

                    FactComboBox {
                        id:         offlineVehicleCombo
                        width:      offlineTypeCombo.width
                        fact:       QGroundControl.offlineEditingVehicleType
                        indexModel: false
                    }
                }

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    visible:  offlineVehicleCombo.currentText != "Multicopter"

                    QGCLabel {
                        text:               qsTr("Cruise speed:")
                        width:              hoverSpeedLabel.width
                        anchors.baseline:   cruiseSpeedField.baseline
                    }


                    FactTextField {
                        id:                 cruiseSpeedField
                        width:              offlineTypeCombo.width
                        fact:               QGroundControl.offlineEditingCruiseSpeed
                        enabled:            true
                    }
                }

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    visible:  offlineVehicleCombo.currentText != "Fixedwing"

                    QGCLabel {
                        id:                 hoverSpeedLabel
                        text:               qsTr("Hover speed:")
                        width:              baseFontLabel.width
                        anchors.baseline:   hoverSpeedField.baseline
                    }


                    FactTextField {
                        id:                 hoverSpeedField
                        width:              offlineTypeCombo.width
                        fact:               QGroundControl.offlineEditingHoverSpeed
                        enabled:            true
                    }
                }

                Item {
                    height: ScreenTools.defaultFontPixelHeight / 2
                    width:  parent.width
                }
            }
        }
    } // QGCViewPanel
} // QGCView
