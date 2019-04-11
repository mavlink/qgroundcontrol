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

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _percentRemainingAnnounce:    QGroundControl.settingsManager.appSettings.batteryPercentRemainingAnnounce
    property Fact _savePath:                    QGroundControl.settingsManager.appSettings.savePath
    property Fact _appFontPointSize:            QGroundControl.settingsManager.appSettings.appFontPointSize
    property Fact _userBrandImageIndoor:        QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor
    property Fact _userBrandImageOutdoor:       QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor
    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _comboFieldWidth:             ScreenTools.defaultFontPixelWidth * 28
    property real _valueFieldWidth:             ScreenTools.defaultFontPixelWidth * 10
    property Fact _mapProvider:                 QGroundControl.settingsManager.flightMapSettings.mapProvider
    property Fact _mapType:                     QGroundControl.settingsManager.flightMapSettings.mapType
    property Fact _followTarget:                QGroundControl.settingsManager.appSettings.followTarget
    property real _panelWidth:                  _root.width * _internalWidthRatio
    property real _margins:                     ScreenTools.defaultFontPixelWidth

    property string _videoSource:               QGroundControl.settingsManager.videoSettings.videoSource.value
    property bool   _isGst:                     QGroundControl.videoManager.isGStreamer
    property bool   _isUDP:                     _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.udpVideoSource
    property bool   _isRTSP:                    _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.rtspVideoSource
    property bool   _isTCP:                     _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.tcpVideoSource
    property bool   _isMPEGTS:                  _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.mpegtsVideoSource

    property string gpsDisabled: "Disabled"
    property string gpsUdpPort:  "UDP Port"

    readonly property real _internalWidthRatio: 0.8

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      outerItem.height
        contentWidth:       outerItem.width

        Item {
            id:     outerItem
            width:  Math.max(_root.width, settingsColumn.width)
            height: settingsColumn.height

            ColumnLayout {
                id:                         settingsColumn
                anchors.horizontalCenter:   parent.horizontalCenter

                QGCLabel {
                    id:         unitsSectionLabel
                    text:       qsTr("Units")
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
                                text:           qsTr("Language")
                                visible: QGroundControl.settingsManager.appSettings.language.visible
                            }
                            FactComboBox {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.appSettings.language
                                indexModel:             false
                                visible:                QGroundControl.settingsManager.appSettings.language.visible
                            }

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
                            }


                            FactCheckBox {
                                text:       qsTr("Mute all audio output")
                                fact:       _audioMuted
                                visible:    _audioMuted.visible
                                property Fact _audioMuted: QGroundControl.settingsManager.appSettings.audioMuted
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
                    id:         loggingSectionLabel
                    text:       qsTr("Data Persistence")
                }
                Rectangle {
                    Layout.preferredHeight: dataPersistCol.height + (_margins * 2)
                    Layout.preferredWidth:  dataPersistCol.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Layout.fillWidth:       true
                    ColumnLayout {
                        id:                         dataPersistCol
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        spacing:                    _margins * 1.5
                        FactCheckBox {
                            id:         disableDataPersistence
                            text:       qsTr("Disable all data persistence")
                            fact:       _disableDataPersistence
                            visible:    _disableDataPersistence.visible
                            property Fact _disableDataPersistence: QGroundControl.settingsManager.appSettings.disableAllPersistence
                        }
                        QGCLabel {
                            text:       qsTr("When Data Persistence is disabled, all telemetry logging and map tile caching is disabled and not written to disk.")
                            wrapMode:   Text.WordWrap
                            font.pointSize:       ScreenTools.smallFontPointSize
                            Layout.maximumWidth:  logIfNotArmed.visible ? logIfNotArmed.width : disableDataPersistence.width * 1.5
                        }
                    }
                }

                Item { width: 1; height: _margins }
                QGCLabel {
                    text:       qsTr("Telemetry Logs from Vehicle")
                }
                Rectangle {
                    Layout.preferredHeight: loggingCol.height + (_margins * 2)
                    Layout.preferredWidth:  loggingCol.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Layout.fillWidth:       true
                    ColumnLayout {
                        id:                         loggingCol
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        spacing:                    _margins
                        FactCheckBox {
                            id:         promptSaveLog
                            text:       qsTr("Save log after each flight")
                            fact:       _telemetrySave
                            visible:    _telemetrySave.visible
                            enabled:    !disableDataPersistence.checked
                            property Fact _telemetrySave: QGroundControl.settingsManager.appSettings.telemetrySave
                        }
                        FactCheckBox {
                            id:         logIfNotArmed
                            text:       qsTr("Save logs even if vehicle was not armed")
                            fact:       _telemetrySaveNotArmed
                            visible:    _telemetrySaveNotArmed.visible
                            enabled:    promptSaveLog.checked && !disableDataPersistence.checked
                            property Fact _telemetrySaveNotArmed: QGroundControl.settingsManager.appSettings.telemetrySaveNotArmed
                        }
                    }
                }

                Item { width: 1; height: _margins }
                QGCLabel {
                    id:         flyViewSectionLabel
                    text:       qsTr("Fly View")
                    visible:    QGroundControl.settingsManager.flyViewSettings.visible
                }
                Rectangle {
                    Layout.preferredHeight: flyViewCol.height + (_margins * 2)
                    Layout.preferredWidth:  flyViewCol.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    visible:                flyViewSectionLabel.visible
                    Layout.fillWidth:       true

                    ColumnLayout {
                        id:                         flyViewCol
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        spacing:                    _margins

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

                        GridLayout {
                            columns: 2

                            QGCLabel { text: qsTr("Guided Minimum Altitude") }
                            FactTextField {
                                Layout.preferredWidth:  _valueFieldWidth
                                fact:                   QGroundControl.settingsManager.flyViewSettings.guidedMinimumAltitude
                            }

                            QGCLabel { text: qsTr("Guided Maximum Altitude") }
                            FactTextField {
                                Layout.preferredWidth:  _valueFieldWidth
                                fact:                   QGroundControl.settingsManager.flyViewSettings.guidedMaximumAltitude
                            }
                        }
                    }
                }

                Item { width: 1; height: _margins }

                QGCLabel {
                    id:         planViewSectionLabel
                    text:       qsTr("Plan View")
                    visible:    QGroundControl.settingsManager.planViewSettings.visible
                }
                Rectangle {
                    Layout.preferredHeight: planViewCol.height + (_margins * 2)
                    Layout.preferredWidth:  planViewCol.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    visible:                planViewSectionLabel.visible
                    Layout.fillWidth:       true

                    ColumnLayout {
                        id:                         planViewCol
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        spacing:                    _margins

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
                                }

                                onActivated: {
                                    if (index != -1) {
                                        QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = textAt(index);
                                    }
                                }
                                Component.onCompleted: {
                                    model.append({text: gpsDisabled})
                                    model.append({text: gpsUdpPort})

                                    for (var i in QGroundControl.linkManager.serialPorts) {
                                        nmeaPortCombo.model.append({text:QGroundControl.linkManager.serialPorts[i]})
                                    }
                                    var index = nmeaPortCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.valueString);
                                    nmeaPortCombo.currentIndex = index;
                                    if (QGroundControl.linkManager.serialPorts.length === 0) {
                                        nmeaPortCombo.model.append({text: "Serial <none available>"})
                                    }
                                }
                            }

                            QGCLabel {
                                visible:          nmeaPortCombo.currentText !== gpsUdpPort && nmeaPortCombo.currentText !== gpsDisabled
                                text:             qsTr("NMEA GPS Baudrate")
                            }
                            QGCComboBox {
                                visible:                nmeaPortCombo.currentText !== gpsUdpPort && nmeaPortCombo.currentText !== gpsDisabled
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

                            QGCLabel {
                                text:       qsTr("NMEA stream UDP port")
                                visible:    nmeaPortCombo.currentText === gpsUdpPort
                            }
                            FactTextField {
                                visible:                nmeaPortCombo.currentText === gpsUdpPort
                                Layout.preferredWidth:  _valueFieldWidth
                                fact:                   QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
                            }
                        }
                    }
                }

                Item { width: 1; height: _margins }

                QGCLabel {
                    id:         rtkSectionLabel
                    text:       qsTr("RTK GPS")
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
                        Layout.fillWidth:           true
                        anchors.horizontalCenter:   parent.horizontalCenter
                        columns:                    3

                        property var rtkSettings:       QGroundControl.settingsManager.rtkSettings
                        property bool useFixedPosition: rtkSettings.useFixedBasePosition.rawValue
                        property real firstColWidth:    ScreenTools.defaultFontPixelWidth * 3

                        ExclusiveGroup {
                            id: useFixedBasePositionRadioGroup
                        }

                        QGCRadioButton {
                            text:               qsTr("Perform Survey-In")
                            visible:            rtkGrid.rtkSettings.useFixedBasePosition.visible
                            checked:            rtkGrid.rtkSettings.useFixedBasePosition.value == false
                            onClicked:          rtkGrid.rtkSettings.useFixedBasePosition.value = false
                            exclusiveGroup:     useFixedBasePositionRadioGroup
                            Layout.columnSpan:  3
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCLabel {
                            text:       rtkGrid.rtkSettings.surveyInAccuracyLimit.shortDescription
                            visible:    rtkGrid.rtkSettings.surveyInAccuracyLimit.visible
                            enabled:    !rtkGrid.useFixedPosition
                        }
                        FactTextField {
                            fact:                   rtkGrid.rtkSettings.surveyInAccuracyLimit
                            visible:                rtkGrid.rtkSettings.surveyInAccuracyLimit.visible
                            enabled:                !rtkGrid.useFixedPosition
                            Layout.preferredWidth:  _valueFieldWidth
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCLabel {
                            text:       rtkGrid.rtkSettings.surveyInMinObservationDuration.shortDescription
                            visible:    rtkGrid.rtkSettings.surveyInMinObservationDuration.visible
                            enabled:    !rtkGrid.useFixedPosition
                        }
                        FactTextField {
                            fact:                   rtkGrid.rtkSettings.surveyInMinObservationDuration
                            visible:                rtkGrid.rtkSettings.surveyInMinObservationDuration.visible
                            enabled:                !rtkGrid.useFixedPosition
                            Layout.preferredWidth:  _valueFieldWidth
                        }

                        QGCRadioButton {
                            text:               qsTr("Use Specified Base Position")
                            visible:            rtkGrid.rtkSettings.useFixedBasePosition.visible
                            checked:            rtkGrid.rtkSettings.useFixedBasePosition.value == true
                            onClicked:          rtkGrid.rtkSettings.useFixedBasePosition.value = true
                            exclusiveGroup:     useFixedBasePositionRadioGroup
                            Layout.columnSpan:  3
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCLabel {
                            text:       rtkGrid.rtkSettings.fixedBasePositionLatitude.shortDescription
                            visible:    rtkGrid.rtkSettings.fixedBasePositionLatitude.visible
                            enabled:    rtkGrid.useFixedPosition
                        }
                        FactTextField {
                            fact:               rtkGrid.rtkSettings.fixedBasePositionLatitude
                            visible:            rtkGrid.rtkSettings.fixedBasePositionLatitude.visible
                            enabled:            rtkGrid.useFixedPosition
                            Layout.fillWidth:   true
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCLabel {
                            text:           rtkGrid.rtkSettings.fixedBasePositionLongitude.shortDescription
                            visible:        rtkGrid.rtkSettings.fixedBasePositionLongitude.visible
                            enabled:        rtkGrid.useFixedPosition
                        }
                        FactTextField {
                            fact:               rtkGrid.rtkSettings.fixedBasePositionLongitude
                            visible:            rtkGrid.rtkSettings.fixedBasePositionLongitude.visible
                            enabled:            rtkGrid.useFixedPosition
                            Layout.fillWidth:   true
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCLabel {
                            text:           rtkGrid.rtkSettings.fixedBasePositionAltitude.shortDescription
                            visible:        rtkGrid.rtkSettings.fixedBasePositionAltitude.visible
                            enabled:        rtkGrid.useFixedPosition
                        }
                        FactTextField {
                            fact:               rtkGrid.rtkSettings.fixedBasePositionAltitude
                            visible:            rtkGrid.rtkSettings.fixedBasePositionAltitude.visible
                            enabled:            rtkGrid.useFixedPosition
                            Layout.fillWidth:   true
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCLabel {
                            text:           rtkGrid.rtkSettings.fixedBasePositionAccuracy.shortDescription
                            visible:        rtkGrid.rtkSettings.fixedBasePositionAccuracy.visible
                            enabled:        rtkGrid.useFixedPosition
                        }
                        FactTextField {
                            fact:               rtkGrid.rtkSettings.fixedBasePositionAccuracy
                            visible:            rtkGrid.rtkSettings.fixedBasePositionAccuracy.visible
                            enabled:            rtkGrid.useFixedPosition
                            Layout.fillWidth:   true
                        }

                        Item { width: rtkGrid.firstColWidth; height: 1 }
                        QGCButton {
                            text:               qsTr("Save Current Base Position")
                            enabled:            QGroundControl.gpsRtk && QGroundControl.gpsRtk.valid.value
                            Layout.columnSpan:  2

                            onClicked: {
                                rtkGrid.rtkSettings.fixedBasePositionLatitude.rawValue =    QGroundControl.gpsRtk.currentLatitude.rawValue
                                rtkGrid.rtkSettings.fixedBasePositionLongitude.rawValue =   QGroundControl.gpsRtk.currentLongitude.rawValue
                                rtkGrid.rtkSettings.fixedBasePositionAltitude.rawValue =    QGroundControl.gpsRtk.currentAltitude.rawValue
                                rtkGrid.rtkSettings.fixedBasePositionAccuracy.rawValue =    QGroundControl.gpsRtk.currentAccuracy.rawValue
                            }
                        }
                    }
                }

                Item { width: 1; height: _margins }

                QGCLabel {
                    id:         videoSectionLabel
                    text:       qsTr("Video")
                    visible:    QGroundControl.settingsManager.videoSettings.visible && !QGroundControl.videoManager.autoStreamConfigured
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
                            text:                   qsTr("Video Source")
                            visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
                        }
                        FactComboBox {
                            id:                     videoSource
                            Layout.preferredWidth:  _comboFieldWidth
                            indexModel:             false
                            fact:                   QGroundControl.settingsManager.videoSettings.videoSource
                            visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
                        }

                        QGCLabel {
                            text:                   qsTr("UDP Port")
                            visible:                (_isUDP || _isMPEGTS)  && QGroundControl.settingsManager.videoSettings.udpPort.visible
                        }
                        FactTextField {
                            Layout.preferredWidth:  _comboFieldWidth
                            fact:                   QGroundControl.settingsManager.videoSettings.udpPort
                            visible:                (_isUDP || _isMPEGTS) && QGroundControl.settingsManager.videoSettings.udpPort.visible
                        }

                        QGCLabel {
                            text:                   qsTr("RTSP URL")
                            visible:                _isRTSP && QGroundControl.settingsManager.videoSettings.rtspUrl.visible
                        }
                        FactTextField {
                            Layout.preferredWidth:  _comboFieldWidth
                            fact:                   QGroundControl.settingsManager.videoSettings.rtspUrl
                            visible:                _isRTSP && QGroundControl.settingsManager.videoSettings.rtspUrl.visible
                        }

                        QGCLabel {
                            text:                   qsTr("TCP URL")
                            visible:                _isTCP && QGroundControl.settingsManager.videoSettings.tcpUrl.visible
                        }
                        FactTextField {
                            Layout.preferredWidth:  _comboFieldWidth
                            fact:                   QGroundControl.settingsManager.videoSettings.tcpUrl
                            visible:                _isTCP && QGroundControl.settingsManager.videoSettings.tcpUrl.visible
                        }
                        QGCLabel {
                            text:                   qsTr("Aspect Ratio")
                            visible:                _isGst && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                        }
                        FactTextField {
                            Layout.preferredWidth:  _comboFieldWidth
                            fact:                   QGroundControl.settingsManager.videoSettings.aspectRatio
                            visible:                _isGst && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                        }

                        QGCLabel {
                            text:                   qsTr("Disable When Disarmed")
                            visible:                _isGst && QGroundControl.settingsManager.videoSettings.disableWhenDisarmed.visible
                        }
                        FactCheckBox {
                            text:                   ""
                            fact:                   QGroundControl.settingsManager.videoSettings.disableWhenDisarmed
                            visible:                _isGst && QGroundControl.settingsManager.videoSettings.disableWhenDisarmed.visible
                        }
                    }
                }

                Item { width: 1; height: _margins }

                QGCLabel {
                    id:                             videoRecSectionLabel
                    text:                           qsTr("Video Recording")
                    visible:                        (QGroundControl.settingsManager.videoSettings.visible && _isGst) || QGroundControl.videoManager.autoStreamConfigured
                }
                Rectangle {
                    Layout.preferredWidth:          videoRecCol.width  + (_margins * 2)
                    Layout.preferredHeight:         videoRecCol.height + (_margins * 2)
                    Layout.fillWidth:               true
                    color:                          qgcPal.windowShade
                    visible:                        videoRecSectionLabel.visible

                    GridLayout {
                        id:                         videoRecCol
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        Layout.fillWidth:           false
                        columns:                    2

                        QGCLabel {
                            text:                   qsTr("Auto-Delete Files")
                            visible:                QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
                        }
                        FactCheckBox {
                            text:                   ""
                            fact:                   QGroundControl.settingsManager.videoSettings.enableStorageLimit
                            visible:                QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
                        }

                        QGCLabel {
                            text:                   qsTr("Max Storage Usage")
                            visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
                        }
                        FactTextField {
                            Layout.preferredWidth:  _comboFieldWidth
                            fact:                   QGroundControl.settingsManager.videoSettings.maxVideoSize
                            visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
                        }

                        QGCLabel {
                            text:                   qsTr("Video File Format")
                            visible:                QGroundControl.settingsManager.videoSettings.recordingFormat.visible
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
    }
}
