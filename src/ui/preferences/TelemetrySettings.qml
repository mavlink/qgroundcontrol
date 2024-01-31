/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import QGroundControl.Palette

SettingsPage {
    property var    _settingsManager:           QGroundControl.settingsManager
    property var    _appSettings:               _settingsManager.appSettings
    property bool   _disableAllDataPersistence: _appSettings.disableAllPersistence.rawValue
    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property string _notConnectedStr:           qsTr("Not Connected")
    property bool   _isAPM:                     _activeVehicle ? _activeVehicle.apmFirmware : true
    property bool   _showAPMStreamRates:        QGroundControl.apmFirmwareSupported && _settingsManager.apmMavlinkStreamRateSettings.visible && _isAPM
    property var     _apmStartMavlinkStreams:   _appSettings.apmStartMavlinkStreams

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Ground Station")

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth * 2

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("MAVLink System ID:")
            }

            QGCTextField {
                text:               QGroundControl.mavlinkSystemID.toString()
                numericValuesOnly:  true
                onEditingFinished: {
                    console.log("text", text)
                    QGroundControl.mavlinkSystemID = parseInt(text)
                }
            }
        }

        QGCCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Emit heartbeat")
            checked:            QGroundControl.multiVehicleManager.gcsHeartBeatEnabled
            onClicked:          QGroundControl.multiVehicleManager.gcsHeartBeatEnabled = checked
        }

        QGCCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Only connect to vehicle with same MAVLink protocol version")
            checked:            QGroundControl.isVersionCheckEnabled
            onClicked:          QGroundControl.isVersionCheckEnabled = checked
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Logging")
        visible:            !_disableAllDataPersistence

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Save log after each flight")
            fact:               _telemetrySave
            visible:            fact.visible
            property Fact _telemetrySave: _appSettings.telemetrySave
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Save logs even if vehicle was not armed")
            fact:               _telemetrySaveNotArmed
            visible:            fact.visible
            enabled:            _appSettings.telemetrySave.rawValue
            property Fact _telemetrySaveNotArmed: _appSettings.telemetrySaveNotArmed
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Save CSV log of telemetry data")
            fact:               _saveCsvTelemetry
            visible:            fact.visible
            property Fact _saveCsvTelemetry: _appSettings.saveCsvTelemetry
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Stream Rates (ArduPilot Only)")
        visible:            _showAPMStreamRates

        QGCCheckBoxSlider {
            id:                 controllerByVehicleCheckBox
            Layout.fillWidth:   true
            text:               qsTr("Controlled By vehicle")
            checked:            !_apmStartMavlinkStreams.rawValue
            onClicked:          _apmStartMavlinkStreams.rawValue = !checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Raw Sensors")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRateRawSensors
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Extended Status")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRateExtendedStatus
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("RC Channels")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRateRCChannels
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Position")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRatePosition
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Extra 1")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRateExtra1
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Extra 2")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRateExtra2
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Extra 3")
            fact:               _settingsManager.apmMavlinkStreamRateSettings.streamRateExtra3
            indexModel:         false
            enabled:            !controllerByVehicleCheckBox.checked
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("MAVLink Forwarding")

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Enable")
            fact:               _appSettings.forwardMavlink
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    ScreenTools.defaultFontPixelWidth * 20
            label:                      qsTr("Host name")
            fact:                       _appSettings.forwardMavlinkHostName
            visible:                    fact.visible
            enabled:                    _appSettings.forwardMavlink.rawValue
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Link Status (Current Vehicle))")

        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Total messages sent (computed)")
            labelText:          _activeVehicle ? _activeVehicle.mavlinkSentCount : _notConnectedStr
        }

        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Total messages received")
            labelText:          _activeVehicle ? _activeVehicle.mavlinkReceivedCount : _notConnectedStr
        }

        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Total message loss")
            labelText:          _activeVehicle ? _activeVehicle.mavlinkLossCount : _notConnectedStr
        }

        LabelledLabel {
            Layout.fillWidth:   true
            label:              qsTr("Loss rate:")
            labelText:          _activeVehicle ? _activeVehicle.mavlinkLossPercent.toFixed(0) + '%' : _notConnectedStr
        }
    }
}
