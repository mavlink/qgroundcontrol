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

ColumnLayout {
    spacing: _rowSpacing

    function saveSettings() {
        // No need
    }

    GridLayout {
        columns:    2
        columnSpacing:  _colSpacing
        rowSpacing:     _rowSpacing

        QGCLabel { text: qsTr("Mode") }
        RowLayout {
            Layout.preferredWidth: _secondColumnWidth
            spacing: _colSpacing

            QGCRadioButton {
                text: qsTr("Classic")
                checked: subEditConfig.mode === 0  // ModeClassic = 0
                onClicked: subEditConfig.mode = 0
            }

            QGCRadioButton {
                text: qsTr("BLE")
                checked: subEditConfig.mode === 1  // ModeLowEnergy = 1
                onClicked: subEditConfig.mode = 1
            }
        }

        QGCLabel { text: qsTr("Device") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.deviceName || qsTr("None selected")
        }

        QGCLabel { text: qsTr("Address") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.address || qsTr("N/A")
        }

        // BLE-specific settings
        QGCLabel {
            text: qsTr("Service UUID")
            visible: subEditConfig.mode === 1  // ModeLowEnergy
        }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.serviceUuid
            placeholderText:        qsTr("Auto-detect")
            visible:                subEditConfig.mode === 1
            onTextChanged:          subEditConfig.serviceUuid = text
        }
    }

    QGCLabel { text: subEditConfig.mode === 1 ? qsTr("BLE Devices") : qsTr("Bluetooth Devices") }

    ScrollView {
        Layout.fillWidth:       true
        Layout.preferredHeight: Math.min(repeaterColumn.height, ScreenTools.defaultFontPixelHeight * 10)
        clip:                   true

        Column {
            id:         repeaterColumn
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelHeight * 0.5

            Repeater {
                model: subEditConfig.nameList

                delegate: QGCButton {
                    text:                   modelData
                    width:                  _secondColumnWidth
                    checkable:              true
                    autoExclusive:          true

                    onClicked: {
                        if (modelData !== "") {
                            subEditConfig.setDevice(modelData)
                        }
                    }
                }
            }

            QGCLabel {
                text:       qsTr("No devices found")
                visible:    subEditConfig.nameList.length === 0 && !subEditConfig.scanning
                width:      _secondColumnWidth
                horizontalAlignment: Text.AlignHCenter
                color:      qgcPal.warningText
            }

            BusyIndicator {
                visible:    subEditConfig.scanning
                running:    visible
                width:      _secondColumnWidth
                height:     ScreenTools.defaultFontPixelHeight * 2
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

    QGCLabel {
        text:               qsTr("Note: BLE devices require service UUID for connection")
        visible:            subEditConfig.mode === 1
        wrapMode:           Text.WordWrap
        Layout.fillWidth:   true
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.warningText
    }
}
