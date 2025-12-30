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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    spacing: _colSpacing

    function saveSettings() {
        console.log(logField.text)
        subEditConfig.filename = logField.text
    }

    QGCLabel { text: qsTr("Log File") }

    QGCTextField {
        id: logField
        Layout.preferredWidth: _secondColumnWidth
        text: subEditConfig.filename
    }

    QGCButton {
        text: qsTr("Browse")
        onClicked: filePicker.openForLoad()
    }

    QGCFileDialog {
        id: filePicker
        title: qsTr("Select Telemetery Log")
        nameFilters: [ qsTr("Telemetry Logs (*.%1)").arg(_logFileExtension), qsTr("All Files (*)") ]
        folder: QGroundControl.settingsManager.appSettings.telemetrySavePath

        property string _logFileExtension: QGroundControl.settingsManager.appSettings.telemetryFileExtension

        onAcceptedForLoad: (file) => {
            logField.text = file
            close()
        }
    }
}
