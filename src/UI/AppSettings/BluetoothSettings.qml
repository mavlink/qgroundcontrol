import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: _rowSpacing

    function saveSettings() {
        // No need
    }

    GridLayout {
        columns:    2
        columnSpacing:  _colSpacing
        rowSpacing:     _rowSpacing

        QGCLabel { text: qsTr("Device") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.deviceName
        }

        QGCLabel { text: qsTr("Address") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.address
        }
    }

    QGCLabel { text: qsTr("Bluetooth Devices") }

    Repeater {
        model: subEditConfig.nameList

        delegate: QGCButton {
            text:                   modelData
            Layout.preferredWidth: _secondColumnWidth
            autoExclusive:          true

            onClicked: {
                checked = true
                if (modelData !== "")
                    subEditConfig.setDevice(modelData)
            }
        }
    }

    RowLayout {
        Layout.alignment:   Qt.AlignCenter
        spacing:            _colSpacing

        QGCButton {
            text:       qsTr("Scan")
            enabled:    !subEditConfig.scanning
            onClicked:  subEditConfig.startScan()
        }

        QGCButton {
            text:       qsTr("Stop")
            enabled:    subEditConfig.scanning
            onClicked:  subEditConfig.stopScan()
        }
    }
}
