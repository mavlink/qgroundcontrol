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
import QtMultimedia             5.5
import QtQuick.Layouts          1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce:    QGroundControl.batteryPercentRemainingAnnounce
    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 15
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 30

    readonly property string _requiresRestart:  qsTr("(Requires Restart)")

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
                width:              qgcView.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                //-----------------------------------------------------------------
                //-- Units
                Item {
                    width:              qgcView.width * 0.8
                    height:             unitLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             unitLabel
                        text:           qsTr("Units (Requires Restart)")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:         unitsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         unitsCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                width:              _labelWidth
                                anchors.baseline:   distanceUnitsCombo.baseline
                                text:               qsTr("Distance:")
                            }
                            FactComboBox {
                                id:                 distanceUnitsCombo
                                width:              _editFieldWidth
                                fact:               QGroundControl.distanceUnits
                                indexModel:         false
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                width:              _labelWidth
                                anchors.baseline:   areaUnitsCombo.baseline
                                text:               qsTr("Area:")
                            }
                            FactComboBox {
                                id:                 areaUnitsCombo
                                width:              _editFieldWidth
                                fact:               QGroundControl.areaUnits
                                indexModel:         false
                            }
                        }
                        Row {
                            spacing:                ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                width:              _labelWidth
                                anchors.baseline:   speedUnitsCombo.baseline
                                text:               qsTr("Speed:")
                            }
                            FactComboBox {
                                id:                 speedUnitsCombo
                                width:              _editFieldWidth
                                fact:               QGroundControl.speedUnits
                                indexModel:         false
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Miscelanous
                Item {
                    width:              qgcView.width * 0.8
                    height:             miscLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             miscLabel
                        text:           qsTr("Miscelaneous")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:         miscCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         miscCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
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
                                text:                   _requiresRestart
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
                                anchors.verticalCenter: parent.verticalCenter
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
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        //-----------------------------------------------------------------
                        //-- Virtual joystick settings
                        QGCCheckBox {
                            text:       qsTr("Virtual Joystick")
                            checked:    QGroundControl.virtualTabletJoystick
                            onClicked:  QGroundControl.virtualTabletJoystick = checked
                            visible:    QGroundControl.corePlugin.options.enableVirtualJoystick
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
                                width:              _labelWidth
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
                                anchors.baseline:   paletteCombo.baseline
                                text:               qsTr("UI Style:")
                                width:              _labelWidth
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
                    }
                }
                //-----------------------------------------------------------------
                //-- Autoconnect settings
                Item {
                    width:              qgcView.width * 0.8
                    height:             autoConnectLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:            QGroundControl.corePlugin.options.enableAutoConnectOptions
                    QGCLabel {
                        id:             autoConnectLabel
                        text:           qsTr("AutoConnect to the following devices:")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:         autoConnectCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    visible:        QGroundControl.corePlugin.options.enableAutoConnectOptions
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         autoConnectCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        //-----------------------------------------------------------------
                        //-- Autoconnect settings
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
                                text:       qsTr("LibrePilot")
                                checked:    QGroundControl.linkManager.autoconnectLibrePilot
                                onClicked:  QGroundControl.linkManager.autoconnectLibrePilot = checked
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
                    }
                }
                //-----------------------------------------------------------------
                //-- Video Source
                Item {
                    width:              qgcView.width * 0.8
                    height:             videoLabel.height
                    visible:            QGroundControl.corePlugin.options.enableVideoSourceOptions
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             videoLabel
                        text:           qsTr("Video (Requires Restart)")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:         videoCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    visible:        QGroundControl.corePlugin.options.enableVideoSourceOptions
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         videoCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                anchors.baseline:   videoSource.baseline
                                text:               qsTr("Video Source:")
                                width:              _labelWidth
                            }
                            QGCComboBox {
                                id:                 videoSource
                                width:              _editFieldWidth
                                model:              QGroundControl.videoManager.videoSourceList
                                Component.onCompleted: {
                                    var index = videoSource.find(QGroundControl.videoManager.videoSource)
                                    if (index >= 0) {
                                        videoSource.currentIndex = index
                                    }
                                }
                                onActivated: {
                                    if (index != -1) {
                                        currentIndex = index
                                        QGroundControl.videoManager.videoSource = model[index]
                                    }
                                }
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 0
                            QGCLabel {
                                anchors.baseline:   udpField.baseline
                                text:               qsTr("UDP Port:")
                                width:              _labelWidth
                            }
                            QGCTextField {
                                id:                 udpField
                                width:              _editFieldWidth
                                text:               QGroundControl.videoManager.udpPort
                                validator:          IntValidator {bottom: 1024; top: 65535;}
                                inputMethodHints:   Qt.ImhDigitsOnly
                                onEditingFinished: {
                                    QGroundControl.videoManager.udpPort = parseInt(text)
                                }
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 1
                            QGCLabel {
                                anchors.baseline:   rtspField.baseline
                                text:               qsTr("RTSP URL:")
                                width:              _labelWidth
                            }
                            QGCTextField {
                                id:                 rtspField
                                width:              _editFieldWidth
                                text:               QGroundControl.videoManager.rtspURL
                                onEditingFinished: {
                                    QGroundControl.videoManager.rtspURL = text
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    text:                       qsTr("QGroundControl Version: " + QGroundControl.qgcVersion)
                }
            } // settingsColumn
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
