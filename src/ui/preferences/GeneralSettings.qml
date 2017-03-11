/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
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
    property Fact _appFontPointSize:            QGroundControl.settingsManager.appSettings.appFontPointSize
    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 15
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 30
    property Fact _telemPath:                   QGroundControl.settingsManager.appSettings.telemetrySavePath
    property Fact _videoPath:                   QGroundControl.settingsManager.videoSettings.videoSavePath

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
                            visible: _appFontPointSize.visible
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                id:     baseFontLabel
                                text:   qsTr("Font size:")
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
                                        if (_appFontPointSize.value > _appFontPointSize.min) {
                                            _appFontPointSize.value = _appFontPointSize.value - 1
                                        }
                                    }
                                }
                                FactTextField {
                                    id:     baseFontEdit
                                    width:  _editFieldWidth - (decrementButton.width * 2) - (baseFontRow.spacing * 2)
                                    fact:   QGroundControl.settingsManager.appSettings.appFontPointSize
                                }
                                QGCButton {
                                    width:  height
                                    height: baseFontEdit.height
                                    text:   "+"
                                    onClicked: {
                                        if (_appFontPointSize.value < _appFontPointSize.max) {
                                            _appFontPointSize.value = _appFontPointSize.value + 1
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
                            visible: QGroundControl.settingsManager.appSettings.indoorPalette.visible
                            QGCLabel {
                                anchors.baseline:   paletteCombo.baseline
                                text:               qsTr("Color scheme:")
                                width:              _labelWidth
                            }
                            FactComboBox {
                                id:         paletteCombo
                                width:      _editFieldWidth
                                fact:       QGroundControl.settingsManager.appSettings.indoorPalette
                                indexModel: false
                            }
                        }
                        //-----------------------------------------------------------------
                        //-- Audio preferences
                        FactCheckBox {
                            text:       qsTr("Mute all audio output")
                            fact:       _audioMuted
                            visible:    _audioMuted.visible

                            property Fact _audioMuted: QGroundControl.settingsManager.appSettings.audioMuted
                        }

                        //-----------------------------------------------------------------
                        //-- Save telemetry log
                        FactCheckBox {
                            id:         promptSaveLog
                            text:       qsTr("Save telemetry log after each flight")
                            fact:       _telemetrySave
                            visible:    !ScreenTools.isMobile && _telemetrySave.visible

                            property Fact _telemetrySave: QGroundControl.settingsManager.appSettings.telemetrySave
                        }

                        //-----------------------------------------------------------------
                        //-- Save even if not armed
                        FactCheckBox {
                            text:       qsTr("Save telemetry log even if vehicle was not armed")
                            fact:       _telemetrySaveNotArmed
                            visible:    !ScreenTools.isMobile && _telemetrySaveNotArmed.visible
                            enabled:    promptSaveLog.checked

                            property Fact _telemetrySaveNotArmed: QGroundControl.settingsManager.appSettings.telemetrySaveNotArmed
                        }

                        //-----------------------------------------------------------------
                        //-- Telemetry save path
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.settingsManager.appSettings.telemetrySavePath.visible

                            QGCLabel {
                                anchors.baseline:   telemBrowse.baseline
                                text:               qsTr("Telemetry save path:")
                                enabled:            promptSaveLog.checked
                            }
                            QGCLabel {
                                anchors.baseline:   telemBrowse.baseline
                                text:               _telemPath.value == "" ? qsTr("<not set>") : _telemPath.value
                                enabled:            promptSaveLog.checked
                            }
                            QGCButton {
                                id:         telemBrowse
                                text:       "Browse"
                                enabled:    promptSaveLog.checked
                                onClicked:  telemDialog.visible = true

                                FileDialog {
                                    id:             telemDialog
                                    title:          "Choose a location to save telemetry files."
                                    folder:         "file://" + _telemPath.value
                                    selectFolder:   true
                                    onAccepted:     _telemPath.value = QGroundControl.urlToLocalFile(telemDialog.fileUrl)
                                }
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
                        FactCheckBox {
                            text:       qsTr("Virtual Joystick")
                            visible:    _virtualJoystick.visible
                            fact:       _virtualJoystick

                            property Fact _virtualJoystick: QGroundControl.settingsManager.appSettings.virtualJoystick
                        }
                        //-----------------------------------------------------------------
                        //-- Default mission item altitude
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude.visible
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
                                    folder:         "file://" + _autoLoadDir.value
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
                            visible:    QGroundControl.settingsManager.videoSettings.videoSource.visible
                            QGCLabel {
                                anchors.baseline:   videoSource.baseline
                                text:               qsTr("Video Source:")
                                width:              _labelWidth
                            }
                            FactComboBox {
                                id:         videoSource
                                width:      _editFieldWidth
                                indexModel: false
                                fact:       QGroundControl.settingsManager.videoSettings.videoSource
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.settingsManager.videoSettings.udpPort.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 0
                            QGCLabel {
                                anchors.baseline:   udpField.baseline
                                text:               qsTr("UDP Port:")
                                width:              _labelWidth
                            }
                            FactTextField {
                                id:                 udpField
                                width:              _editFieldWidth
                                fact:               QGroundControl.settingsManager.videoSettings.udpPort
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.settingsManager.videoSettings.rtspUrl.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 1
                            QGCLabel {
                                anchors.baseline:   rtspField.baseline
                                text:               qsTr("RTSP URL:")
                                width:              _labelWidth
                            }
                            FactTextField {
                                id:                 rtspField
                                width:              _editFieldWidth
                                fact:               QGroundControl.settingsManager.videoSettings.rtspUrl
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex < 2 && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                            QGCLabel {
                                anchors.baseline:   aspectField.baseline
                                text:               qsTr("Aspect Ratio:")
                                width:              _labelWidth
                            }
                            FactTextField {
                                id:                 aspectField
                                width:              _editFieldWidth
                                fact:               QGroundControl.settingsManager.videoSettings.aspectRatio
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex < 2 && QGroundControl.settingsManager.videoSettings.gridLines.visible
                            QGCLabel {
                                anchors.baseline:   gridField.baseline
                                text:               qsTr("Grid Lines:")
                                width:              _labelWidth
                            }
                            FactComboBox {
                                id:                 gridField
                                width:              _editFieldWidth
                                fact:               QGroundControl.settingsManager.videoSettings.gridLines
                            }
                        }
                        Row {
                            spacing:    ScreenTools.defaultFontPixelWidth
                            visible:    QGroundControl.settingsManager.videoSettings.videoSavePath.visible && QGroundControl.videoManager.isGStreamer && QGroundControl.videoManager.recordingEnabled

                            QGCLabel {
                                anchors.baseline:   videoBrowse.baseline
                                text:               qsTr("Save path:")
                                enabled:            promptSaveLog.checked
                            }
                            QGCLabel {
                                anchors.baseline:   videoBrowse.baseline
                                text:               _videoPath.value == "" ? qsTr("<not set>") : _videoPath.value
                            }
                            QGCButton {
                                id:         videoBrowse
                                text:       "Browse"
                                onClicked:  videoDialog.visible = true

                                FileDialog {
                                    id:             videoDialog
                                    title:          "Choose a location to save video files."
                                    folder:         "file://" + _videoPath.value
                                    selectFolder:   true
                                    onAccepted:     _videoPath.value = QGroundControl.urlToLocalFile(videoDialog.fileUrl)
                                }
                            }
                        }
                    }
                } // Video Source - Rectangle

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    text:                       qsTr("QGroundControl Version: " + QGroundControl.qgcVersion)
                }
            } // settingsColumn
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
