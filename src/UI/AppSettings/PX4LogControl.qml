import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    Layout.fillWidth: true
    heading:          qsTr("MAVLink 2.0 Logging (PX4 Pro Only)")
    visible:          QGroundControl.corePlugin.options.showPX4LogTransferOptions && _isPX4

    property var  _activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property bool _isPX4:                  _activeVehicle ? _activeVehicle.px4Firmware : true
    property var  _mavlinkLogManager:      _activeVehicle ? _activeVehicle.mavlinkLogManager : null
    property bool _disableDataPersistence: QGroundControl.settingsManager.appSettings.disableAllPersistence.rawValue

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:               qsTr("Manual Start/Stop:")
            Layout.fillWidth:   true
        }

        QGCButton {
            text:    qsTr("Start Logging")
            enabled: _mavlinkLogManager && !_mavlinkLogManager.logRunning && _mavlinkLogManager.canStartLog && !_disableDataPersistence
            onClicked: _mavlinkLogManager.startLogging()
        }

        QGCButton {
            text:    qsTr("Stop Logging")
            enabled: _mavlinkLogManager && _mavlinkLogManager.logRunning && !_disableDataPersistence
            onClicked: _mavlinkLogManager.stopLogging()
        }
    }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text:     qsTr("Enable automatic logging")
        checked:  _mavlinkLogManager ? _mavlinkLogManager.enableAutoStart : false
        enabled:  !_disableDataPersistence
        onClicked: _mavlinkLogManager.enableAutoStart = checked
    }
}
