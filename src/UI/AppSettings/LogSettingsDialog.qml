import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

QGCPopupDialog {
    buttons: Dialog.Close
    title: qsTr("Logging Settings")

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: true
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight
        width: ScreenTools.defaultFontPixelWidth * 50

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Status")
            visible: qgcLogging.hasError

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                Rectangle {
                    color: "red"
                    implicitHeight: implicitWidth
                    implicitWidth: ScreenTools.defaultFontPixelHeight
                    radius: implicitWidth / 2
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: qgcLogging.lastError || qsTr("A logging error has occurred")
                    wrapMode: Text.WordWrap
                }

                QGCButton {
                    text: qsTr("Clear Error")

                    onClicked: qgcLogging.clearError()
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Disk Logging")
            headingDescription: qsTr("Log files are saved to: %1").arg(QGroundControl.settingsManager.appSettings.crashSavePath)

            QGCCheckBoxSlider {
                Layout.fillWidth: true
                text: qsTr("Enable disk logging")

                Binding on checked {
                    value: qgcLogging.diskLoggingEnabled
                }

                onClicked: qgcLogging.diskLoggingEnabled = checked
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Remote Logging")
            headingDescription: qsTr("Sends log messages to a remote server. UDP is fire-and-forget, TCP is reliable, Auto starts with UDP and falls back to TCP.")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCCheckBoxSlider {
                    id: remoteSwitch

                    Layout.fillWidth: true
                    text: qsTr("Enable remote logging")

                    Binding on checked {
                        value: qgcLogging.remoteLoggingEnabled
                    }

                    onClicked: qgcLogging.remoteLoggingEnabled = checked
                }

                Rectangle {
                    Accessible.name: qgcLogging.remoteTcpConnected ? qsTr("TCP Connected") : qsTr("TCP Disconnected")
                    Accessible.role: Accessible.Indicator
                    border.color: qgcPal.text
                    border.width: 1
                    color: qgcLogging.remoteTcpConnected ? "green" : (qgcLogging.remoteLoggingEnabled ? "yellow" : qgcPal.windowShade)
                    implicitHeight: implicitWidth
                    implicitWidth: ScreenTools.defaultFontPixelHeight * 0.75
                    radius: implicitWidth / 2
                    visible: qgcLogging.remoteProtocol !== LogManager.UDP

                    QGCMouseArea {
                        ToolTip.text: qgcLogging.remoteTcpConnected ? qsTr("TCP Connected") : qsTr("TCP Disconnected")
                        ToolTip.visible: containsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                enabled: remoteSwitch.checked
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Endpoint (host:port)")
                }

                QGCTextField {
                    id: endpointField

                    Layout.fillWidth: true
                    placeholderText: qsTr("e.g., 192.168.1.100:5000")

                    Binding on text {
                        value: qgcLogging.remoteEndpoint
                        when: !endpointField.activeFocus
                    }

                    onEditingFinished: qgcLogging.remoteEndpoint = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                enabled: remoteSwitch.checked
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Vehicle ID")
                }

                QGCTextField {
                    id: vehicleIdField

                    Layout.fillWidth: true
                    placeholderText: qsTr("Optional identifier")

                    Binding on text {
                        value: qgcLogging.remoteVehicleId
                        when: !vehicleIdField.activeFocus
                    }

                    onEditingFinished: qgcLogging.remoteVehicleId = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                enabled: remoteSwitch.checked
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Protocol")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCComboBox {
                    currentIndex: qgcLogging.remoteProtocol
                    model: [qsTr("UDP"), qsTr("TCP"), qsTr("Auto Fallback")]

                    onActivated: index => {
                        qgcLogging.remoteProtocol = index;
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: qgcLogging.remoteLoggingEnabled

                QGCLabel {
                    text: {
                        const kb = qgcLogging.remoteBytesSent / 1024;
                        return kb < 1024 ? qsTr("%1 KB sent").arg(Math.round(kb)) : qsTr("%1 MB sent").arg((kb / 1024).toFixed(1));
                    }
                }

                QGCButton {
                    text: qsTr("Reset")

                    onClicked: qgcLogging.resetRemoteBytesSent()
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: "red"
                text: qgcLogging.lastRemoteError
                visible: qgcLogging.lastRemoteError !== ""
                wrapMode: Text.WordWrap
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("TLS Encryption")
            headingDescription: qsTr("Encrypts TCP connections. Disable certificate verification only for self-signed certificates in trusted environments.")
            visible: remoteSwitch.checked && qgcLogging.remoteProtocol !== LogManager.UDP

            QGCCheckBoxSlider {
                id: tlsSwitch

                Layout.fillWidth: true
                text: qsTr("Enable TLS")

                Binding on checked {
                    value: qgcLogging.remoteTlsEnabled
                }

                onClicked: qgcLogging.remoteTlsEnabled = checked
            }

            QGCCheckBoxSlider {
                Layout.fillWidth: true
                enabled: tlsSwitch.checked
                text: qsTr("Verify server certificate")

                Binding on checked {
                    value: qgcLogging.remoteTlsVerifyPeer
                }

                onClicked: qgcLogging.remoteTlsVerifyPeer = checked
            }

            RowLayout {
                id: caRow

                Layout.fillWidth: true
                enabled: tlsSwitch.checked
                spacing: ScreenTools.defaultFontPixelWidth

                property string _caPath: ""

                QGCLabel {
                    text: qsTr("CA Certificate")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCLabel {
                    color: qgcPal.colorGrey
                    text: caRow._caPath || qsTr("Not loaded")
                }

                QGCButton {
                    text: qsTr("Browse")

                    onClicked: caCertDialog.openForLoad()
                }

                QGCFileDialog {
                    id: caCertDialog

                    nameFilters: [qsTr("PEM files (*.pem)"), qsTr("All Files (*)")]
                    title: qsTr("Select CA Certificate")

                    onAcceptedForLoad: file => {
                        if (qgcLogging.loadRemoteTlsCaCertificates(file)) {
                            caRow._caPath = file;
                        }
                    }
                }
            }

            RowLayout {
                id: clientCertRow

                Layout.fillWidth: true
                enabled: tlsSwitch.checked
                spacing: ScreenTools.defaultFontPixelWidth

                property string _certPath: ""
                property string _keyPath: ""

                QGCLabel {
                    text: qsTr("Client Certificate")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCButton {
                    text: clientCertRow._certPath ? qsTr("Cert \u2713") : qsTr("Cert")

                    onClicked: clientCertDialog.openForLoad()
                }

                QGCButton {
                    text: clientCertRow._keyPath ? qsTr("Key \u2713") : qsTr("Key")

                    onClicked: clientKeyDialog.openForLoad()
                }

                QGCButton {
                    enabled: clientCertRow._certPath !== "" && clientCertRow._keyPath !== ""
                    text: qsTr("Load")

                    onClicked: qgcLogging.loadRemoteTlsClientCertificate(clientCertRow._certPath, clientCertRow._keyPath)
                }

                QGCFileDialog {
                    id: clientCertDialog

                    nameFilters: [qsTr("PEM files (*.pem)"), qsTr("All Files (*)")]
                    title: qsTr("Select Client Certificate")

                    onAcceptedForLoad: file => {
                        clientCertRow._certPath = file;
                    }
                }

                QGCFileDialog {
                    id: clientKeyDialog

                    nameFilters: [qsTr("PEM files (*.pem)"), qsTr("All Files (*)")]
                    title: qsTr("Select Client Key")

                    onAcceptedForLoad: file => {
                        clientCertRow._keyPath = file;
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: "red"
                text: qgcLogging.lastTlsError
                visible: qgcLogging.lastTlsError !== ""
                wrapMode: Text.WordWrap
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Compression")
            headingDescription: qsTr("Compresses log data using zlib. Level 1 is fastest, level 9 provides best compression.")
            visible: remoteSwitch.checked

            QGCCheckBoxSlider {
                id: compressionSwitch

                Layout.fillWidth: true
                text: qsTr("Enable compression")

                Binding on checked {
                    value: qgcLogging.remoteCompressionEnabled
                }

                onClicked: qgcLogging.remoteCompressionEnabled = checked
            }

            RowLayout {
                Layout.fillWidth: true
                enabled: compressionSwitch.checked
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Compression level")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: Math.round(compressionSlider.value)
                }

                QGCSlider {
                    id: compressionSlider

                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                    from: 1
                    stepSize: 1
                    to: 9

                    Binding on value {
                        value: qgcLogging.remoteCompressionLevel
                    }

                    onMoved: qgcLogging.remoteCompressionLevel = value
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Actions")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text: qsTr("Clear Log")

                    onClicked: qgcLogging.clear()
                }

                QGCButton {
                    enabled: qgcLogging.diskLoggingEnabled
                    text: qsTr("Flush to Disk")

                    onClicked: qgcLogging.flush()
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Buffer")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Max entries")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: Math.round(bufferSlider.value).toLocaleString()
                }

                QGCSlider {
                    id: bufferSlider

                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                    from: 10000
                    stepSize: 10000
                    to: 500000

                    Binding on value {
                        value: qgcLogging.model.maxEntries
                    }

                    onMoved: qgcLogging.model.maxEntries = value
                }
            }
        }
    }
}
