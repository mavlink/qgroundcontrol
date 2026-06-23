import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

QGCPopupDialog {
    id:                     escCalibrationDlg
    title:                  qsTr("ESC Calibration")
    buttons:                Dialog.Ok
    acceptButtonEnabled:    false

    readonly property string _highlightPrefix: "<font color=\"" + qgcPal.warningText + "\">"
    readonly property string _highlightSuffix: "</font>"

    Connections {
        target: controller

        function onOldFirmware() {
            textLabel.text = _highlightPrefix + qsTr("ESC Calibration failed. ") + _highlightSuffix +
                qsTr("%1 cannot perform ESC Calibration with this version of firmware. You will need to upgrade to a newer firmware.").arg(QGroundControl.appName)
            escCalibrationDlg.acceptButtonEnabled = true
        }

        function onNewerFirmware() {
            textLabel.text = _highlightPrefix + qsTr("ESC Calibration failed. ") + _highlightSuffix +
                qsTr("%1 cannot perform ESC Calibration with this version of firmware. You will need to upgrade %1.").arg(QGroundControl.appName)
            escCalibrationDlg.acceptButtonEnabled = true
        }

        function onDisconnectBattery() {
            textLabel.text = _highlightPrefix + qsTr("ESC Calibration failed. ") + _highlightSuffix +
                qsTr("You must disconnect the battery prior to performing ESC Calibration. Disconnect your battery and try again.")
            escCalibrationDlg.acceptButtonEnabled = true
        }

        function onConnectBattery() {
            textLabel.text = _highlightPrefix + qsTr("WARNING: Props must be removed from vehicle prior to performing ESC calibration.") + _highlightSuffix +
                qsTr(" Connect the battery now and calibration will begin.")
        }

        function onBatteryConnected() {
            textLabel.text = qsTr("Performing calibration. This will take a few seconds..")
        }

        function onCalibrationFailed(errorMessage) {
            escCalibrationDlg.acceptButtonEnabled = true
            textLabel.text = _highlightPrefix + qsTr("ESC Calibration failed. ") + _highlightSuffix + errorMessage
        }

        function onCalibrationSuccess() {
            escCalibrationDlg.acceptButtonEnabled = true
            textLabel.text = qsTr("Calibration complete. You can disconnect your battery now if you like.")
        }
    }

    Component.onCompleted: controller.calibrateEsc()

    ColumnLayout {
        QGCLabel {
            id:                     textLabel
            wrapMode:               Text.WordWrap
            text:                   qsTr("Starting ESC calibration...")
            Layout.fillWidth:       true
            Layout.maximumWidth:    mainWindow.width / 2
        }
    }
}
