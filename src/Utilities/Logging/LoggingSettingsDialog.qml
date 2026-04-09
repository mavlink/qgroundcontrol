import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Logging
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

QGCPopupDialog {
    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    readonly property var appSettings: QGroundControl.settingsManager.appSettings
    property bool _exportFiltered: false
    property int _selectedFormatIndex: 0
    readonly property var exportFormats: [
        { label: qsTr("Text (.txt)"),  filters: [qsTr("Text files (*.txt)"), qsTr("All Files (*)")] },
        { label: qsTr("JSON (.json)"), filters: [qsTr("JSON files (*.json)"), qsTr("All Files (*)")] },
        { label: qsTr("CSV (.csv)"),   filters: [qsTr("CSV files (*.csv)"), qsTr("All Files (*)")] },
        { label: qsTr("JSONL (.jsonl)"), filters: [qsTr("JSON Lines files (*.jsonl)"), qsTr("All Files (*)")] }
    ]
    readonly property var sink: LogManager.remoteSink

    buttons: Dialog.Close
    title: qsTr("Logging Settings")

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight / 2
        width: maxContentAvailableWidth

        // ── Disk Logging ─────────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Disk Logging")

            QGCCheckBoxSlider {
                Layout.fillWidth: true
                checked: LogManager.diskLoggingEnabled
                text: qsTr("Enable disk logging")

                onClicked: LogManager.diskLoggingEnabled = checked
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.colorGrey
                text: qsTr("Log files are saved to: %1").arg(appSettings.logSavePath)
                visible: LogManager.diskLoggingEnabled
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: LogManager.diskLoggingEnabled

                QGCLabel {
                    text: qsTr("Flush on level:")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCComboBox {
                    readonly property var _levelNames: [qsTr("Off"), qsTr("Warning"), qsTr("Critical")]
                    readonly property var _levelValues: [-1, LogEntry.Warning, LogEntry.Critical]

                    currentIndex: _levelValues.indexOf(LogManager.flushOnLevel)
                    model: _levelNames
                    sizeToContents: true

                    onActivated: index => {
                        LogManager.flushOnLevel = _levelValues[index];
                    }
                }
            }
        }

        // ── Remote Logging ───────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Remote Logging")

            FactCheckBoxSlider {
                Layout.fillWidth: true
                fact: appSettings.remoteLoggingEnabled
                text: qsTr("Enable remote logging")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: appSettings.remoteLoggingEnabled.rawValue

                QGCLabel {
                    text: qsTr("Host:")
                }

                FactTextField {
                    Layout.fillWidth: true
                    fact: appSettings.remoteLoggingHost
                }

                QGCLabel {
                    text: qsTr("Port:")
                }

                FactTextField {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                    fact: appSettings.remoteLoggingPort
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: appSettings.remoteLoggingEnabled.rawValue

                QGCLabel {
                    text: qsTr("Protocol:")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCComboBox {
                    currentIndex: appSettings.remoteLoggingProtocol.rawValue
                    model: [qsTr("UDP"), qsTr("TCP"), qsTr("Auto Fallback")]
                    sizeToContents: true

                    onActivated: index => {
                        appSettings.remoteLoggingProtocol.rawValue = index;
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.colorGrey
                text: qsTr("UDP is fire-and-forget, TCP is reliable, Auto Fallback starts with UDP and switches to TCP on failures.")
                visible: appSettings.remoteLoggingEnabled.rawValue
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: appSettings.remoteLoggingEnabled.rawValue

                QGCLabel {
                    text: qsTr("Vehicle ID:")
                }

                FactTextField {
                    Layout.fillWidth: true
                    fact: appSettings.remoteLoggingVehicleId
                    placeholderText: qsTr("Optional identifier")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: sink && sink.enabled

                QGCLabel {
                    text: {
                        const kb = sink.bytesSent / 1024;
                        return kb < 1024 ? qsTr("%1 KB sent").arg(Math.round(kb)) : qsTr("%1 MB sent").arg((kb / 1024).toFixed(1));
                    }
                }

                QGCButton {
                    text: qsTr("Reset")

                    onClicked: sink.resetBytesSent()
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.warningText
                text: sink ? qsTr("Error: %1").arg(sink.lastError) : ""
                visible: sink && sink.lastError !== ""
                wrapMode: Text.WordWrap
            }
        }

        // ── TLS Encryption ───────────────────────────────────────
        SettingsGroupLayout {
            id: tlsGroup

            property string _tlsCaPath: ""
            property string _tlsCertPath: ""
            property string _tlsKeyPath: ""

            Layout.fillWidth: true
            heading: qsTr("TLS Encryption")
            visible: appSettings.remoteLoggingEnabled.rawValue && appSettings.remoteLoggingProtocol.rawValue !== LogRemoteSink.UDP

            FactCheckBoxSlider {
                Layout.fillWidth: true
                fact: appSettings.remoteLoggingTlsEnabled
                text: qsTr("Enable TLS")
            }

            FactCheckBoxSlider {
                Layout.fillWidth: true
                fact: appSettings.remoteLoggingTlsVerifyPeer
                text: qsTr("Verify server certificate")
                visible: appSettings.remoteLoggingTlsEnabled.rawValue
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: appSettings.remoteLoggingTlsEnabled.rawValue

                QGCLabel {
                    text: qsTr("CA Certificate")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCLabel {
                    color: qgcPal.colorGrey
                    text: tlsGroup._tlsCaPath || qsTr("Not loaded")
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
                        if (sink.loadTlsCaCertificates(file))
                            tlsGroup._tlsCaPath = file;
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: appSettings.remoteLoggingTlsEnabled.rawValue

                QGCLabel {
                    text: qsTr("Client Certificate")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCButton {
                    text: tlsGroup._tlsCertPath ? qsTr("Cert \u2713") : qsTr("Cert")

                    onClicked: clientCertDialog.openForLoad()
                }

                QGCButton {
                    text: tlsGroup._tlsKeyPath ? qsTr("Key \u2713") : qsTr("Key")

                    onClicked: clientKeyDialog.openForLoad()
                }

                QGCButton {
                    enabled: tlsGroup._tlsCertPath !== "" && tlsGroup._tlsKeyPath !== ""
                    text: qsTr("Load")

                    onClicked: sink.loadTlsClientCertificate(tlsGroup._tlsCertPath, tlsGroup._tlsKeyPath)
                }

                QGCFileDialog {
                    id: clientCertDialog

                    nameFilters: [qsTr("PEM files (*.pem)"), qsTr("All Files (*)")]
                    title: qsTr("Select Client Certificate")

                    onAcceptedForLoad: file => {
                        tlsGroup._tlsCertPath = file;
                    }
                }

                QGCFileDialog {
                    id: clientKeyDialog

                    nameFilters: [qsTr("PEM files (*.pem)"), qsTr("All Files (*)")]
                    title: qsTr("Select Client Key")

                    onAcceptedForLoad: file => {
                        tlsGroup._tlsKeyPath = file;
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.warningText
                text: sink ? qsTr("TLS Error: %1").arg(sink.lastTlsError) : ""
                visible: sink && sink.lastTlsError !== ""
                wrapMode: Text.WordWrap
            }
        }

        // ── Compression ──────────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Compression")
            visible: appSettings.remoteLoggingEnabled.rawValue

            FactCheckBoxSlider {
                Layout.fillWidth: true
                fact: appSettings.remoteLoggingCompressionEnabled
                text: qsTr("Enable compression")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: appSettings.remoteLoggingCompressionEnabled.rawValue

                QGCLabel {
                    text: qsTr("Level")
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
                    value: appSettings.remoteLoggingCompressionLevel.rawValue

                    onMoved: appSettings.remoteLoggingCompressionLevel.rawValue = value
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.colorGrey
                text: qsTr("Compresses log data using zlib. Level 1 is fastest, level 9 provides best compression.")
                visible: appSettings.remoteLoggingCompressionEnabled.rawValue
                wrapMode: Text.WordWrap
            }
        }

        // ── Console Buffer ───────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Console Buffer")

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
                    value: LogManager.model.maxEntries

                    onMoved: LogManager.model.maxEntries = value
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.colorGrey
                text: qsTr("Number of log entries kept in memory for the console view. Higher values use more RAM.")
                wrapMode: Text.WordWrap
            }
        }

        // ── Log History ──────────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Log History")
            visible: LogManager.logStore.isOpen

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.colorGrey
                text: qsTr("Session: %1 (%2 entries)").arg(LogManager.logStore.sessionId).arg(LogManager.logStore.entryCount)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Browse session:")
                }

                QGCComboBox {
                    id: sessionCombo

                    Layout.fillWidth: true
                    model: LogManager.historyModel.availableSessions
                    sizeToContents: true

                    Component.onCompleted: {
                        if (count > 0)
                            currentIndex = count - 1;
                    }
                    onActivated: LogManager.historyModel.sessionFilter = currentText
                    onModelChanged: {
                        if (count > 0) {
                            const prev = currentText;
                            const idx = find(prev);
                            currentIndex = idx >= 0 ? idx : count - 1;
                        }
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                color: qgcPal.colorGrey
                text: qsTr("%1 entries in selected session").arg(LogManager.historyModel.totalResults)
                visible: sessionCombo.currentText !== ""
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                visible: sessionCombo.currentText !== ""

                QGCButton {
                    text: qsTr("Export Session")

                    onClicked: historyExportDialog.openForSave()
                }

                QGCButton {
                    text: qsTr("Delete Session")

                    onClicked: deleteConfirmDialogFactory.open()
                }
            }

            QGCPopupDialogFactory {
                id: deleteConfirmDialogFactory

                dialogComponent: Component {
                    QGCSimpleMessageDialog {
                        title: qsTr("Confirm Delete")
                        text: qsTr("Delete session \"%1\" and all its log entries?").arg(sessionCombo.currentText)
                        buttons: Dialog.Yes | Dialog.No

                        onAccepted: LogManager.logStore.deleteSession(sessionCombo.currentText)
                    }
                }
            }

            QGCFileDialog {
                id: historyExportDialog

                folder: QGroundControl.settingsManager.appSettings.logSavePath
                nameFilters: exportFormats[_selectedFormatIndex].filters
                title: qsTr("Export session log")

                onAcceptedForSave: file => {
                    LogManager.logStore.exportSession(sessionCombo.currentText, file, _selectedFormatIndex);
                }
            }
        }

        // ── GStreamer ─────────────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("GStreamer")
            visible: appSettings.gstDebugLevel.userVisible

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Debug level:")
                }

                Item {
                    Layout.fillWidth: true
                }

                FactComboBox {
                    fact: appSettings.gstDebugLevel
                    sizeToContents: true
                }
            }
        }

        // ── Export ────────────────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Export")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Format:")
                }

                Item {
                    Layout.fillWidth: true
                }

                QGCComboBox {
                    id: exportFormatCombo

                    currentIndex: _selectedFormatIndex
                    model: exportFormats.map(f => f.label)

                    onActivated: _selectedFormatIndex = currentIndex
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text: qsTr("Save Log")

                    onClicked: {
                        _exportFiltered = false;
                        saveFileDialog.openForSave();
                    }
                }

                QGCButton {
                    text: qsTr("Save Filtered")
                    visible: LogManager.model.filterLevel > LogEntry.Debug || LogManager.model.filterCategory !== "" || LogManager.model.filterText !== ""

                    onClicked: {
                        _exportFiltered = true;
                        saveFileDialog.openForSave();
                    }
                }
            }
        }

        // ── Actions ──────────────────────────────────────────────
        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Actions")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text: qsTr("Clear Log")

                    onClicked: LogManager.model.clear()
                }

                QGCButton {
                    enabled: LogManager.diskLoggingEnabled
                    text: qsTr("Flush to Disk")

                    onClicked: LogManager.flush()
                }
            }
        }
    }

    QGCFileDialog {
        id: saveFileDialog

        folder: QGroundControl.settingsManager.appSettings.logSavePath
        nameFilters: exportFormats[_selectedFormatIndex].filters
        title: _exportFiltered ? qsTr("Save filtered log") : qsTr("Save app log")

        onAcceptedForSave: file => {
            if (_exportFiltered)
                LogManager.writeFilteredMessages(file, _selectedFormatIndex);
            else
                LogManager.writeMessages(file, _selectedFormatIndex);
        }
    }
}
