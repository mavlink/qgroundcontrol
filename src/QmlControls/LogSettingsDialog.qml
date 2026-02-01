import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

QGCPopupDialog {
    title:      qsTr("Logging Settings")
    buttons:    Dialog.Close

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        width:      ScreenTools.defaultFontPixelWidth * 50
        spacing:    ScreenTools.defaultFontPixelHeight

        // Error Status
        SettingsGroupLayout {
            heading:            qsTr("Status")
            Layout.fillWidth:   true
            visible:            qgcLogging.hasError

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                Rectangle {
                    width:  ScreenTools.defaultFontPixelHeight
                    height: width
                    radius: width / 2
                    color:  "red"
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("A logging error has occurred")
                    wrapMode:           Text.WordWrap
                }

                QGCButton {
                    text:       qsTr("Clear Error")
                    onClicked:  qgcLogging.clearError()
                }
            }
        }

        // Disk Logging
        SettingsGroupLayout {
            heading:            qsTr("Disk Logging")
            Layout.fillWidth:   true

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Enable disk logging")
                }

                Item { Layout.fillWidth: true }

                Switch {
                    checked:    qgcLogging.diskLoggingEnabled
                    onClicked:  qgcLogging.diskLoggingEnabled = checked
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Log files are saved to: %1").arg(QGroundControl.settingsManager.appSettings.logSavePath)
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.text
                opacity:            0.7
            }
        }

        // Remote Logging - Basic
        SettingsGroupLayout {
            heading:            qsTr("Remote Logging")
            Layout.fillWidth:   true

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Enable remote logging")
                }

                Item { Layout.fillWidth: true }

                // Connection status indicator
                Rectangle {
                    width:          ScreenTools.defaultFontPixelHeight * 0.75
                    height:         width
                    radius:         width / 2
                    color:          qgcLogging.remoteTcpConnected ? "green" : (qgcLogging.remoteLoggingEnabled ? "yellow" : qgcPal.windowShade)
                    border.color:   qgcPal.text
                    border.width:   1
                    visible:        qgcLogging.remoteProtocol !== 0  // Not UDP

                    QGCMouseArea {
                        anchors.fill:   parent
                        hoverEnabled:   true

                        ToolTip.visible: containsMouse
                        ToolTip.text:    qgcLogging.remoteTcpConnected ? qsTr("TCP Connected") : qsTr("TCP Disconnected")
                    }
                }

                Switch {
                    id:         remoteSwitch
                    checked:    qgcLogging.remoteLoggingEnabled
                    onClicked:  qgcLogging.remoteLoggingEnabled = checked
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                enabled:            remoteSwitch.checked

                QGCLabel {
                    text: qsTr("Endpoint (host:port)")
                }

                QGCTextField {
                    id:                 endpointField
                    Layout.fillWidth:   true
                    text:               qgcLogging.remoteEndpoint
                    placeholderText:    qsTr("e.g., 192.168.1.100:5000")

                    onEditingFinished: {
                        qgcLogging.remoteEndpoint = text
                    }
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                enabled:            remoteSwitch.checked

                QGCLabel {
                    text: qsTr("Vehicle ID")
                }

                QGCTextField {
                    Layout.fillWidth:   true
                    text:               qgcLogging.remoteVehicleId
                    placeholderText:    qsTr("Optional identifier")

                    onEditingFinished: {
                        qgcLogging.remoteVehicleId = text
                    }
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                enabled:            remoteSwitch.checked

                QGCLabel {
                    text: qsTr("Protocol")
                }

                Item { Layout.fillWidth: true }

                QGCComboBox {
                    model:          [qsTr("UDP"), qsTr("TCP"), qsTr("Auto Fallback")]
                    currentIndex:   qgcLogging.remoteProtocol
                    onActivated:    (index) => { qgcLogging.remoteProtocol = index }
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Sends log messages to a remote server. UDP is fire-and-forget, TCP is reliable, Auto Fallback starts with UDP and switches to TCP on failures.")
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.text
                opacity:            0.7
            }
        }

        // Remote Logging - TLS
        SettingsGroupLayout {
            heading:            qsTr("TLS Encryption")
            Layout.fillWidth:   true
            visible:            remoteSwitch.checked && qgcLogging.remoteProtocol !== 0  // Not UDP

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Enable TLS")
                }

                Item { Layout.fillWidth: true }

                Switch {
                    id:         tlsSwitch
                    checked:    qgcLogging.remoteTlsEnabled
                    onClicked:  qgcLogging.remoteTlsEnabled = checked
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                enabled:            tlsSwitch.checked

                QGCLabel {
                    text: qsTr("Verify server certificate")
                }

                Item { Layout.fillWidth: true }

                Switch {
                    checked:    qgcLogging.remoteTlsVerifyPeer
                    onClicked:  qgcLogging.remoteTlsVerifyPeer = checked
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Encrypts TCP connections using TLS. Disable certificate verification only for self-signed certificates in trusted environments.")
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.text
                opacity:            0.7
            }
        }

        // Remote Logging - Compression
        SettingsGroupLayout {
            heading:            qsTr("Compression")
            Layout.fillWidth:   true
            visible:            remoteSwitch.checked

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Enable compression")
                }

                Item { Layout.fillWidth: true }

                Switch {
                    id:         compressionSwitch
                    checked:    qgcLogging.remoteCompressionEnabled
                    onClicked:  qgcLogging.remoteCompressionEnabled = checked
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                enabled:            compressionSwitch.checked

                QGCLabel {
                    text: qsTr("Compression level")
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    text: compressionSlider.value
                }

                QGCSlider {
                    id:             compressionSlider
                    from:           1
                    to:             9
                    stepSize:       1
                    value:          qgcLogging.remoteCompressionLevel
                    onMoved:        qgcLogging.remoteCompressionLevel = value
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Compresses log data using zlib. Level 1 is fastest, level 9 provides best compression.")
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.text
                opacity:            0.7
            }
        }

        // Actions
        SettingsGroupLayout {
            heading:            qsTr("Actions")
            Layout.fillWidth:   true

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       qsTr("Clear Log")
                    onClicked:  qgcLogging.clear()
                }

                QGCButton {
                    text:       qsTr("Flush to Disk")
                    enabled:    qgcLogging.diskLoggingEnabled
                    onClicked:  qgcLogging.flush()
                }
            }
        }
    }
}
