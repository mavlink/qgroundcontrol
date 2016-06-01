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

Rectangle {
    id:                 _generalRoot
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce:    QGroundControl.multiVehicleManager.disconnectedVehicle.battery.percentRemainingAnnounce
    property real _firstLabelWidth:             ScreenTools.defaultFontPixelWidth * 16
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 22

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
                font.pointSize: ScreenTools.mediumFontPointSize
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
            //-- Base UI Font Point Size
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    width:              _firstLabelWidth
                    text:               qsTr("Base UI font size:")
                    anchors.verticalCenter: parent.verticalCenter
                }
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        width:              baseFontEdit.height
                        height:             width
                        color:              qgcPal.button
                        QGCLabel {
                            text:           "-"
                            anchors.centerIn: parent
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if(ScreenTools.defaultFontPointSize > 6)
                                    QGroundControl.baseFontPointSize = QGroundControl.baseFontPointSize - 1
                            }
                        }
                    }
                    QGCTextField {
                        id:                 baseFontEdit
                        width:              _editFieldWidth - (height * 2)
                        text:               QGroundControl.baseFontPointSize
                        showUnits:          true
                        unitsLabel:         "pt"
                        maximumLength:      6
                        validator:          DoubleValidator {bottom: 6.0; top: 48.0; decimals: 2;}
                        onEditingFinished: {
                            var point = parseFloat(text)
                            if(point >= 6.0 && point <= 48.0)
                                QGroundControl.baseFontPointSize = point;
                        }
                    }
                    Rectangle {
                        width:              baseFontEdit.height
                        height:             width
                        color:              qgcPal.button
                        QGCLabel {
                            text:           "+"
                            anchors.centerIn: parent
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if(ScreenTools.defaultFontPointSize < 49)
                                    QGroundControl.baseFontPointSize = QGroundControl.baseFontPointSize + 1
                            }
                        }
                    }
                }
                QGCLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    text:               qsTr("(requires reboot to take affect)")
                }
            }

            //-----------------------------------------------------------------
            //-- Units

            Row {
                spacing:    ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    width:              _firstLabelWidth
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
                    text:               qsTr("(requires reboot to take affect)")
                }

            }

            Row {
                spacing:                ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    anchors.baseline:   speedUnitsCombo.baseline
                    width:              _firstLabelWidth
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
                    text:              qsTr("(requires reboot to take affect)")
                }
            }

            //-----------------------------------------------------------------
            //-- Scale on Flight View
            QGCCheckBox {
                text:       qsTr("Show scale on Fly View")
                onClicked: {
                    QGroundControl.flightMapSettings.showScaleOnFlyView = checked
                }
                Component.onCompleted: {
                    checked = QGroundControl.flightMapSettings.showScaleOnFlyView
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
                    width:              _firstLabelWidth
                    text:               qsTr("Map Providers:")
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
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    anchors.baseline:   paletteCombo.baseline
                    width:              _firstLabelWidth
                    text:               qsTr("Style:")
                }
                QGCComboBox {
                    id:                 paletteCombo
                    width:              _editFieldWidth
                    model:              [ qsTr("Indoor"), qsTr("Outdoor") ]
                    currentIndex:       QGroundControl.isDarkStyle ? 0 : 1
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
