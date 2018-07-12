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
    id:                 _qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce:    QGroundControl.settingsManager.appSettings.batteryPercentRemainingAnnounce
    property Fact _savePath:                    QGroundControl.settingsManager.appSettings.savePath
    property Fact _appFontPointSize:            QGroundControl.settingsManager.appSettings.appFontPointSize
    property Fact _userBrandImageIndoor:        QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor
    property Fact _userBrandImageOutdoor:       QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor
    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _comboFieldWidth:             ScreenTools.defaultFontPixelWidth * 25
    property real _valueFieldWidth:             ScreenTools.defaultFontPixelWidth * 8
    property Fact _mapProvider:                 QGroundControl.settingsManager.flightMapSettings.mapProvider
    property Fact _mapType:                     QGroundControl.settingsManager.flightMapSettings.mapType
    property Fact _followTarget:                QGroundControl.settingsManager.appSettings.followTarget
    property real _panelWidth:                  _qgcView.width * _internalWidthRatio
    property real _margins:                     ScreenTools.defaultFontPixelWidth

    readonly property real _internalWidthRatio:          0.8

    readonly property string _requiresRestart:  qsTr("(Requires Restart)")

    QGCPalette { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      outerItem.height
            contentWidth:       outerItem.width

            Item {
                id:     outerItem
                width:  Math.max(panel.width, settingsColumn.width)
                height: settingsColumn.height

                ColumnLayout {
                    id:                         settingsColumn
                    anchors.horizontalCenter:   parent.horizontalCenter

                    QGCLabel {
                        id:         unitsSectionLabel
                        text:       qsTr("Units (Requires Restart)")
                        visible:    QGroundControl.settingsManager.unitsSettings.visible
                    }
                    Rectangle {
                        Layout.preferredHeight: unitsGrid.height + (_margins * 2)
                        Layout.preferredWidth:  unitsGrid.width + (_margins * 2)
                        color:                  qgcPal.windowShade
                        visible:                miscSectionLabel.visible
                        Layout.fillWidth:       true

                        GridLayout {
                            id:                         unitsGrid
                            anchors.topMargin:          _margins
                            anchors.top:                parent.top
                            Layout.fillWidth:           false
                            anchors.horizontalCenter:   parent.horizontalCenter
                            flow:                       GridLayout.TopToBottom
                            rows:                       4

                            Repeater {
                                model: [ qsTr("Distance"), qsTr("Area"), qsTr("Speed"), qsTr("Temperature") ]
                                QGCLabel { text: modelData }
                            }
                            Repeater {
                                model:  [ QGroundControl.settingsManager.unitsSettings.distanceUnits, QGroundControl.settingsManager.unitsSettings.areaUnits, QGroundControl.settingsManager.unitsSettings.speedUnits, QGroundControl.settingsManager.unitsSettings.temperatureUnits ]
                                FactComboBox {
                                    Layout.preferredWidth:  _comboFieldWidth
                                    fact:                   modelData
                                    indexModel:             false
                                }
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        id:         miscSectionLabel
                        text:       qsTr("Miscellaneous")
                        visible:    QGroundControl.settingsManager.appSettings.visible
                    }
                    Rectangle {
                        Layout.preferredWidth:  Math.max(comboGrid.width, miscCol.width) + (_margins * 2)
                        Layout.preferredHeight: (pathRow.visible ? pathRow.y + pathRow.height : miscColItem.y + miscColItem.height)  + (_margins * 2)
                        Layout.fillWidth:       true
                        color:                  qgcPal.windowShade
                        visible:                miscSectionLabel.visible

                        Item {
                            id:                 comboGridItem
                            anchors.margins:    _margins
                            anchors.top:        parent.top
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            height:             comboGrid.height

                            GridLayout {
                                id:                         comboGrid
                                anchors.horizontalCenter:   parent.horizontalCenter
                                columns:                    2

                                QGCLabel {
                                    text:           qsTr("Color Scheme")
                                    visible: QGroundControl.settingsManager.appSettings.indoorPalette.visible
                                }
                                FactComboBox {
                                    Layout.preferredWidth:  _comboFieldWidth
                                    fact:                   QGroundControl.settingsManager.appSettings.indoorPalette
                                    indexModel:             false
                                    visible:                QGroundControl.settingsManager.appSettings.indoorPalette.visible
                                }

                                QGCLabel {
                                    text:       qsTr("Map Provider")
                                    width:      _labelWidth
                                    visible:    _mapProvider.visible
                                }
                                FactComboBox {
                                    Layout.preferredWidth:  _comboFieldWidth
                                    fact:                   _mapProvider
                                    indexModel:             false
                                    visible:                _mapProvider.visible
                                }

                                QGCLabel {
                                    text:       qsTr("Map Type")
                                    visible:    _mapType.visible
                                }
                                FactComboBox {
                                    id:                     mapTypes
                                    Layout.preferredWidth:  _comboFieldWidth
                                    fact:                   _mapType
                                    indexModel:             false
                                    visible:                _mapType.visible
                                    Connections {
                                        target: QGroundControl.settingsManager.flightMapSettings
                                        onMapTypeChanged: {
                                            mapTypes.model = _mapType.enumStrings
                                        }
                                    }
                                }

                                QGCLabel {
                                    text:       qsTr("Stream GCS Position")
                                    visible:    _followTarget.visible
                                }
                                FactComboBox {
                                    Layout.preferredWidth:  _comboFieldWidth
                                    fact:                   _followTarget
                                    indexModel:             false
                                    visible:                _followTarget.visible
                                }
                            }
                        }

                        Item {
                            id:                 miscColItem
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.top:        comboGridItem.bottom
                            height:             miscCol.height

                            ColumnLayout {
                                id:                         miscCol
                                anchors.horizontalCenter:   parent.horizontalCenter
                                spacing:                    _margins

                                RowLayout {
                                    Layout.fillWidth:   false
                                    Layout.alignment:   Qt.AlignHCenter
                                    visible:            _appFontPointSize ? _appFontPointSize.visible : false

                                    QGCLabel {
                                        text:   qsTr("Font Size:")
                                    }
                                    QGCButton {
                                        Layout.preferredWidth:  height
                                        Layout.preferredHeight: baseFontEdit.height
                                        text:                   "-"
                                        onClicked: {
                                            if (_appFontPointSize.value > _appFontPointSize.min) {
                                                _appFontPointSize.value = _appFontPointSize.value - 1
                                            }
                                        }
                                    }
                                    FactTextField {
                                        id:                     baseFontEdit
                                        Layout.preferredWidth:  _valueFieldWidth
                                        fact:                   QGroundControl.settingsManager.appSettings.appFontPointSize
                                    }
                                    QGCButton {
                                        Layout.preferredWidth:  height
                                        Layout.preferredHeight: baseFontEdit.height
                                        text:                   "+"
                                        onClicked: {
                                            if (_appFontPointSize.value < _appFontPointSize.max) {
                                                _appFontPointSize.value = _appFontPointSize.value + 1
                                            }
                                        }
                                    }
                                    QGCLabel {
                                        text: _requiresRestart
                                    }
                                }


                                FactCheckBox {
                                    text:       qsTr("Mute all audio output")
                                    fact:       _audioMuted
                                    visible:    _audioMuted.visible
                                    property Fact _audioMuted: QGroundControl.settingsManager.appSettings.audioMuted
                                }

                                FactCheckBox {
                                    id:         promptSaveLog
                                    text:       qsTr("Save telemetry log after each flight")
                                    fact:       _telemetrySave
                                    visible:    _telemetrySave.visible
                                    property Fact _telemetrySave: QGroundControl.settingsManager.appSettings.telemetrySave
                                }

                                FactCheckBox {
                                    text:       qsTr("Save telemetry log even if vehicle was not armed")
                                    fact:       _telemetrySaveNotArmed
                                    visible:    _telemetrySaveNotArmed.visible
                                    enabled:    promptSaveLog.checked
                                    property Fact _telemetrySaveNotArmed: QGroundControl.settingsManager.appSettings.telemetrySaveNotArmed
                                }

                                FactCheckBox {
                                    text:       qsTr("Use preflight checklist")
                                    fact:       _useChecklist
                                    visible:    _useChecklist.visible

                                    property Fact _useChecklist: QGroundControl.settingsManager.appSettings.useChecklist
                                }

                                FactCheckBox {
                                    text:       qsTr("Virtual Joystick")
                                    visible:    _virtualJoystick.visible
                                    fact:       _virtualJoystick

                                    property Fact _virtualJoystick: QGroundControl.settingsManager.appSettings.virtualJoystick
                                }

                                FactCheckBox {
                                    text:       qsTr("AutoLoad Missions")
                                    fact:       _autoLoad
                                    visible:    _autoLoad.visible

                                    property Fact _autoLoad: QGroundControl.settingsManager.appSettings.autoLoadMissions
                                }

                                QGCCheckBox {
                                    id:         clearCheck
                                    text:       qsTr("Clear all settings on next start")
                                    checked:    false
                                    onClicked: {
                                        checked ? clearDialog.visible = true : QGroundControl.clearDeleteAllSettingsNextBoot()
                                    }
                                    MessageDialog {
                                        id:                 clearDialog
                                        visible:            false
                                        icon:               StandardIcon.Warning
                                        standardButtons:    StandardButton.Yes | StandardButton.No
                                        title:              qsTr("Clear Settings")
                                        text:               qsTr("All saved settings will be reset the next time you start %1. Is this really what you want?").arg(QGroundControl.appName)
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

                                RowLayout {
                                    visible: QGroundControl.settingsManager.appSettings.batteryPercentRemainingAnnounce.visible

                                    QGCCheckBox {
                                        id:         announcePercentCheckbox
                                        text:       qsTr("Announce battery lower than")
                                        checked:    _percentRemainingAnnounce.value !== 0
                                        onClicked: {
                                            if (checked) {
                                                _percentRemainingAnnounce.value = _percentRemainingAnnounce.defaultValueString
                                            } else {
                                                _percentRemainingAnnounce.value = 0
                                            }
                                        }
                                    }
                                    FactTextField {
                                        fact:                   _percentRemainingAnnounce
                                        Layout.preferredWidth:  _valueFieldWidth
                                        enabled:                announcePercentCheckbox.checked
                                    }
                                }

                                RowLayout {
                                    spacing:    ScreenTools.defaultFontPixelWidth
                                    visible:    QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude.visible

                                    QGCLabel { text: qsTr("Default Mission Altitude") }
                                    FactTextField {
                                        Layout.preferredWidth:  _valueFieldWidth
                                        fact:                   QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
                                    }
                                }
                            }
                        }

                        //-----------------------------------------------------------------
                        //-- Save path
                        RowLayout {
                            id:                 pathRow
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.top:        miscColItem.bottom
                            visible:            _savePath.visible && !ScreenTools.isMobile

                            QGCLabel { text: qsTr("Application Load/Save Path") }
                            QGCTextField {
                                Layout.fillWidth:   true
                                readOnly:           true
                                text:               _savePath.rawValue === "" ? qsTr("<not set>") : _savePath.value
                            }
                            QGCButton {
                                text:       qsTr("Browse")
                                onClicked:  savePathBrowseDialog.openForLoad()

                                QGCFileDialog {
                                    id:             savePathBrowseDialog
                                    qgcView:        _qgcView
                                    title:          qsTr("Choose the location to save/load files")
                                    folder:         _savePath.rawValue
                                    selectExisting: true
                                    selectFolder:   true

                                    onAcceptedForLoad: _savePath.rawValue = file
                                }
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        id:         rtkSectionLabel
                        text:       qsTr("RTK GPS (Requires Restart)")
                        visible:    QGroundControl.settingsManager.rtkSettings.visible
                    }
                    Rectangle {
                        Layout.preferredHeight: rtkGrid.height + (_margins * 2)
                        Layout.preferredWidth:  rtkGrid.width + (_margins * 2)
                        color:                  qgcPal.windowShade
                        visible:                rtkSectionLabel.visible
                        Layout.fillWidth:       true

                        GridLayout {
                            id:                         rtkGrid
                            anchors.topMargin:          _margins
                            anchors.top:                parent.top
                            Layout.fillWidth:           false
                            anchors.horizontalCenter:   parent.horizontalCenter
                            columns:                    2

                            QGCLabel { text: qsTr("Survey in accuracy") }
                            FactTextField {
                                Layout.preferredWidth:  _valueFieldWidth
                                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInAccuracyLimit
                            }

                            QGCLabel { text: qsTr("Minimum observation duration") }
                            FactTextField {
                                Layout.preferredWidth:  _valueFieldWidth
                                fact:                   QGroundControl.settingsManager.rtkSettings.surveyInMinObservationDuration
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        id:         autoConnectSectionLabel
                        text:       qsTr("AutoConnect to the following devices")
                        visible:    QGroundControl.settingsManager.autoConnectSettings.visible
                    }
                    Rectangle {
                        Layout.preferredWidth:  autoConnectCol.width + (_margins * 2)
                        Layout.preferredHeight: autoConnectCol.height + (_margins * 2)
                        color:                  qgcPal.windowShade
                        visible:                autoConnectSectionLabel.visible
                        Layout.fillWidth:       true

                        ColumnLayout {
                            id:                 autoConnectCol
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            spacing:            _margins

                            RowLayout {
                                spacing: _margins

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
                                        visible:    modelData.visible
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth:   false
                                Layout.alignment:   Qt.AlignHCenter
                                columns:            2
                                visible:            !ScreenTools.isMobile
                                                    && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.visible
                                                    && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.visible

                                QGCLabel {
                                    text: qsTr("NMEA GPS Device")
                                }
                                QGCComboBox {
                                    id:                     nmeaPortCombo
                                    Layout.preferredWidth:  _comboFieldWidth

                                    model:  ListModel {
                                        ListElement { text: "disabled" }
                                    }

                                    onActivated: {
                                        if (index != -1) {
                                            QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = textAt(index);
                                        }
                                    }
                                    Component.onCompleted: {
                                        for (var i in QGroundControl.linkManager.serialPorts) {
                                            nmeaPortCombo.model.append({text:QGroundControl.linkManager.serialPorts[i]})
                                        }
                                        var index = nmeaPortCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.valueString);
                                        nmeaPortCombo.currentIndex = index;
                                    }
                                }

                                QGCLabel {
                                    text:             qsTr("NMEA GPS Baudrate")
                                }
                                QGCComboBox {
                                    id:                     nmeaBaudCombo
                                    Layout.preferredWidth:  _comboFieldWidth
                                    model:                  [4800, 9600, 19200, 38400, 57600, 115200]

                                    onActivated: {
                                        if (index != -1) {
                                            QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = textAt(index);
                                        }
                                    }
                                    Component.onCompleted: {
                                        var index = nmeaBaudCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.valueString);
                                        nmeaBaudCombo.currentIndex = index;
                                    }
                                }
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        id:         videoSectionLabel
                        text:       qsTr("Video")
                        visible:    QGroundControl.settingsManager.videoSettings.visible
                    }
                    Rectangle {
                        Layout.preferredWidth:  videoGrid.width + (_margins * 2)
                        Layout.preferredHeight: videoGrid.height + (_margins * 2)
                        Layout.fillWidth:       true
                        color:                  qgcPal.windowShade
                        visible:                videoSectionLabel.visible

                        GridLayout {
                            id:                         videoGrid
                            anchors.margins:            _margins
                            anchors.top:                parent.top
                            anchors.horizontalCenter:   parent.horizontalCenter
                            Layout.fillWidth:           false
                            Layout.fillHeight:          false
                            columns:                    2

                            QGCLabel {
                                text:       qsTr("Video Source")
                                visible:    QGroundControl.settingsManager.videoSettings.videoSource.visible
                            }
                            FactComboBox {
                                id:                     videoSource
                                Layout.preferredWidth:  _comboFieldWidth
                                indexModel:             false
                                fact:                   QGroundControl.settingsManager.videoSettings.videoSource
                                visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
                            }

                            QGCLabel {
                                text:       qsTr("UDP Port")
                                visible:    QGroundControl.settingsManager.videoSettings.udpPort.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 1
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.udpPort
                                visible:                QGroundControl.settingsManager.videoSettings.udpPort.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 1
                            }

                            QGCLabel {
                                text:       qsTr("RTSP URL")
                                visible:    QGroundControl.settingsManager.videoSettings.rtspUrl.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 2
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.rtspUrl
                                visible:                QGroundControl.settingsManager.videoSettings.rtspUrl.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 2
                            }

                            QGCLabel {
                                text:       qsTr("TCP URL")
                                visible:    QGroundControl.settingsManager.videoSettings.tcpUrl.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 3
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.tcpUrl
                                visible:                QGroundControl.settingsManager.videoSettings.tcpUrl.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex === 3
                            }

                            QGCLabel {
                                text:       qsTr("Aspect Ratio")
                                visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex && videoSource.currentIndex < 3 && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.aspectRatio
                                visible:                QGroundControl.videoManager.isGStreamer && videoSource.currentIndex && videoSource.currentIndex < 3 && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                            }

                            QGCLabel {
                                text:       qsTr("Disable When Disarmed")
                                visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex && videoSource.currentIndex < 3 && QGroundControl.settingsManager.videoSettings.gridLines.visible
                            }
                            FactCheckBox {
                                text:       ""
                                fact:       QGroundControl.settingsManager.videoSettings.disableWhenDisarmed
                                visible:    QGroundControl.videoManager.isGStreamer && videoSource.currentIndex && videoSource.currentIndex < 3 && QGroundControl.settingsManager.videoSettings.gridLines.visible
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        id:         videoRecSectionLabel
                        text:       qsTr("Video Recording")
                        visible:    QGroundControl.settingsManager.videoSettings.visible && QGroundControl.videoManager.isGStreamer && videoSource.currentIndex && videoSource.currentIndex < 4
                    }
                    Rectangle {
                        Layout.preferredWidth:  videoRecCol.width + (_margins * 2)
                        Layout.preferredHeight: videoRecCol.height + (_margins * 2)
                        Layout.fillWidth:       true
                        color:                  qgcPal.windowShade
                        visible:                videoRecSectionLabel.visible

                        GridLayout {
                            id:                         videoRecCol
                            anchors.margins:            _margins
                            anchors.top:                parent.top
                            anchors.horizontalCenter:   parent.horizontalCenter
                            Layout.fillWidth:           false
                            columns:                    2

                            QGCLabel {
                                text:       qsTr("Auto-Delete Files")
                                visible:    QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
                            }
                            FactCheckBox {
                                text:       ""
                                fact:       QGroundControl.settingsManager.videoSettings.enableStorageLimit
                                visible:    QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
                            }

                            QGCLabel {
                                text:       qsTr("Max Storage Usage")
                                visible:    QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.maxVideoSize
                                visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
                            }

                            QGCLabel {
                                text:       qsTr("Video File Format")
                                visible:    QGroundControl.settingsManager.videoSettings.recordingFormat.visible
                            }
                            FactComboBox {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.recordingFormat
                                visible:                QGroundControl.settingsManager.videoSettings.recordingFormat.visible
                            }
                        }
                    }

                    Item { width: 1; height: _margins; visible: videoRecSectionLabel.visible }

                    QGCLabel {
                        id:         brandImageSectionLabel
                        text:       qsTr("Brand Image")
                        visible:    QGroundControl.settingsManager.brandImageSettings.visible && !ScreenTools.isMobile
                    }
                    Rectangle {
                        Layout.preferredWidth:  brandImageGrid.width + (_margins * 2)
                        Layout.preferredHeight: brandImageGrid.height + (_margins * 2)
                        Layout.fillWidth:       true
                        color:                  qgcPal.windowShade
                        visible:                brandImageSectionLabel.visible

                        GridLayout {
                            id:                 brandImageGrid
                            anchors.margins:    _margins
                            anchors.top:        parent.top
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            columns:            3

                            QGCLabel {
                                text:       qsTr("Indoor Image")
                                visible:    _userBrandImageIndoor.visible
                            }
                            QGCTextField {
                                readOnly:           true
                                Layout.fillWidth:   true
                                text:               _userBrandImageIndoor.valueString.replace("file:///","")
                            }
                            QGCButton {
                                text:       qsTr("Browse")
                                onClicked:  userBrandImageIndoorBrowseDialog.openForLoad()

                                QGCFileDialog {
                                    id:             userBrandImageIndoorBrowseDialog
                                    qgcView:        _qgcView
                                    title:          qsTr("Choose custom brand image file")
                                    folder:         _userBrandImageIndoor.rawValue.replace("file:///","")
                                    selectExisting: true
                                    selectFolder:   false

                                    onAcceptedForLoad: _userBrandImageIndoor.rawValue = "file:///" + file
                                }
                            }

                            QGCLabel {
                                text:       qsTr("Outdoor Image")
                                visible:    _userBrandImageOutdoor.visible
                            }
                            QGCTextField {
                                readOnly:           true
                                Layout.fillWidth:   true
                                text:               _userBrandImageOutdoor.valueString.replace("file:///","")
                            }
                            QGCButton {
                                text:       qsTr("Browse")
                                onClicked:  userBrandImageOutdoorBrowseDialog.openForLoad()

                                QGCFileDialog {
                                    id:             userBrandImageOutdoorBrowseDialog
                                    qgcView:        _qgcView
                                    title:          qsTr("Choose custom brand image file")
                                    folder:         _userBrandImageOutdoor.rawValue.replace("file:///","")
                                    selectExisting: true
                                    selectFolder:   false

                                    onAcceptedForLoad: _userBrandImageOutdoor.rawValue = "file:///" + file
                                }
                            }

                            QGCButton {
                                text:               qsTr("Reset Default Brand Image")
                                Layout.columnSpan:  3
                                Layout.alignment:   Qt.AlignHCenter

                                onClicked:  {
                                    _userBrandImageIndoor.rawValue = ""
                                    _userBrandImageOutdoor.rawValue = ""
                                }
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        text:               qsTr("%1 Version").arg(QGroundControl.appName)
                        Layout.alignment:   Qt.AlignHCenter
                    }
                    QGCLabel {
                        text:               QGroundControl.qgcVersion
                        Layout.alignment:   Qt.AlignHCenter
                    }
                } // settingsColumn
            }
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
