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
import QGroundControl.SettingsManager       1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce:    QGroundControl.settingsManager.appSettings.batteryPercentRemainingAnnounce
    property Fact _autoLoadDir:                 QGroundControl.settingsManager.appSettings.missionAutoLoadDir
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
                    width:                      qgcView.width * 0.8
                    height:                     unitLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.unitsSettings.visible
                    QGCLabel {
                        id:             unitLabel
                        text:           qsTr("Units (Requires Restart)")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     unitsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      qgcView.width * 0.8
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.unitsSettings.visible
                    Column {
                        id:         unitsCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent

                        Repeater {
                            id:     unitsRepeater
                            model:  [ QGroundControl.settingsManager.unitsSettings.distanceUnits, QGroundControl.settingsManager.unitsSettings.areaUnits, QGroundControl.settingsManager.unitsSettings.speedUnits ]

                            property var names: [ qsTr("Distance:"), qsTr("Area:"), qsTr("Speed:") ]

                            Row {
                                spacing:    ScreenTools.defaultFontPixelWidth
                                visible:    modelData.visible

                                QGCLabel {
                                    width:              _labelWidth
                                    anchors.baseline:   factCombo.baseline
                                    text:               unitsRepeater.names[index]
                                }
                                FactComboBox {
                                    id:                 factCombo
                                    width:              _editFieldWidth
                                    fact:               modelData
                                    indexModel:         false
                                }
                            }
                        }
                    }
                }

                //-----------------------------------------------------------------
                //-- Miscellanous
                Item {
                    width:                      qgcView.width * 0.8
                    height:                     miscLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    QGCLabel {
                        id:             miscLabel
                        text:           qsTr("Miscellaneous")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     miscCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      qgcView.width * 0.8
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    Column {
                        id:         miscCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        //-----------------------------------------------------------------
                        //-- Base UI Font Point Size
                        Row {
                            visible: QGroundControl.corePlugin.options.defaultFontPointSize < 1.0
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
                        //-----------------------------------------------------------------
                        //-- Audio preferences
                        FactCheckBox {
                            text:   qsTr("Mute all audio output")
                            fact:   QGroundControl.settingsManager.appSettings.audioMuted
                        }
                        //-----------------------------------------------------------------
                        //-- Prompt Save Log
                        FactCheckBox {
                            id:         promptSaveLog
                            text:       qsTr("Prompt to save Flight Data Log after each flight")
                            fact:       QGroundControl.settingsManager.appSettings.promptFlightTelemetrySave
                            visible:    !ScreenTools.isMobile
                        }
                        //-----------------------------------------------------------------
                        //-- Prompt Save even if not armed
                        FactCheckBox {
                            text:       qsTr("Prompt to save Flight Data Log even if vehicle was not armed")
                            fact:       QGroundControl.settingsManager.appSettings.promptFlightTelemetrySaveNotArmed
                            visible:    !ScreenTools.isMobile
                            enabled:    promptSaveLog.checked
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
                        //-- Default mission item altitude
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                anchors.baseline:   defaultItemAltitudeField.baseline
                                text:               qsTr("Default mission item altitude:")
                            }
                            FactTextField {
                                id:     defaultItemAltitudeField
                                fact:   QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
                            }
                        }
                        //-----------------------------------------------------------------
                        //-- Mission AutoLoad
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCCheckBox {
                                id:                     autoLoadCheckbox
                                anchors.verticalCenter: parent.verticalCenter
                                text:                   qsTr("AutoLoad mission directory:")
                                checked:                _autoLoadDir.valueString

                                onClicked: {
                                    if (checked) {
                                        _autoLoadDir.rawValue = QGroundControl.urlToLocalFile(autoloadDirPicker.shortcuts.home)
                                    } else {
                                        _autoLoadDir.rawValue = ""
                                    }
                                }
                            }
                            FactTextField {
                                id:                     autoLoadDirField
                                width:                  _editFieldWidth
                                enabled:                autoLoadCheckbox.checked
                                anchors.verticalCenter: parent.verticalCenter
                                fact:                   _autoLoadDir
                            }
                            QGCButton {
                                text:       qsTr("Browse")
                                onClicked:  autoloadDirPicker.visible = true

                                FileDialog {
                                    id:             autoloadDirPicker
                                    title:          qsTr("Choose the location of mission file.")
                                    folder:         shortcuts.home
                                    selectFolder:   true
                                    onAccepted:     _autoLoadDir.rawValue = QGroundControl.urlToLocalFile(autoloadDirPicker.fileUrl)
                                }
                            }
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
                    }
                }

                //-----------------------------------------------------------------
                //-- Autoconnect settings
                Item {
                    width:                      qgcView.width * 0.8
                    height:                     autoConnectLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.autoConnectSettings.visible
                    QGCLabel {
                        id:             autoConnectLabel
                        text:           qsTr("AutoConnect to the following devices:")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     autoConnectCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      qgcView.width * 0.8
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.autoConnectSettings.visible

                    Column {
                        id:         autoConnectCol
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth * 2

                            Repeater {
                                id:     autoConnectRepeater
                                model:  [ QGroundControl.settingsManager.autoConnectSettings.autoConnectPixhawk,
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectSiKRadio,
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectPX4Flow,
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectLibrePilot,
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectUDP,
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                                ]

                                property var names: [ qsTr("Pixhawk"), qsTr("SiK Radio"), qsTr("PX4 Flow"), qsTr("LibrePilot"), qsTr("UDP"), qsTr("RTK GPS") ]

                                FactCheckBox {
                                    text:       autoConnectRepeater.names[index]
                                    fact:       modelData
                                    visible:    !ScreenTools.isiOS && modelData.visible
                                }
                            }
                        }
                    }
                }

                //-----------------------------------------------------------------
                //-- Video Source
                Item {
                    width:                      qgcView.width * 0.8
                    height:                     videoLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.videoSettings.visible
                    QGCLabel {
                        id:             videoLabel
                        text:           qsTr("Video (Requires Restart)")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     videoCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      qgcView.width * 0.8
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.videoSettings.visible
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
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.videoManager.isGStreamer && QGroundControl.videoManager.recordingEnabled
                            QGCLabel {
                                anchors.baseline:   pathField.baseline
                                text:               qsTr("Save Path:")
                                width:              _labelWidth
                            }
                            QGCTextField {
                                id:                 pathField
                                width:              _editFieldWidth
                                readOnly:           true
                                text:               QGroundControl.videoManager.videoSavePath
                            }
                            QGCButton {
                                text:       "Browse"
                                onClicked:  videoLocationFileDialog.visible = true

                                FileDialog {
                                    id:             videoLocationFileDialog
                                    title:          "Choose a location to save video files."
                                    folder:         shortcuts.home
                                    selectFolder:   true
                                    onAccepted:     QGroundControl.videoManager.setVideoSavePathByUrl(fileDialog.fileUrl)
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
