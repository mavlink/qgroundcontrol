import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    property var controller: QGroundControl.gpsManager.recordingController
    property string exportFolder: QGroundControl.settingsManager.appSettings.logSavePath

    heading: qsTr("GPS Recording")
    headingDescription: qsTr("Record receiver data for diagnosis and replay. Recordings may contain precise locations. Starting a recording replaces the previous capture.")
    visible: root.controller !== null

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsRecordingStatus"
        text: !root.controller ? qsTr("Recording unavailable") : root.controller.limitReached ? qsTr("Recording stopped at its size limit. Export the captured data before starting again.") : root.controller.recording ? qsTr("Recording: %1 events, %2 bytes").arg(root.controller.eventCount).arg(root.controller.bytesRecorded) : root.controller.hasRecording ? qsTr("Ready to export: %1 events, %2 bytes").arg(root.controller.eventCount).arg(root.controller.bytesRecorded) : qsTr("No recording")
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth

        QGCButton {
            enabled: root.controller !== null
            objectName: "gpsRecordingToggle"
            text: root.controller && root.controller.recording ? qsTr("Stop recording") : qsTr("Start recording")

            onClicked: {
                if (root.controller) {
                    if (root.controller.recording) {
                        root.controller.stop();
                    } else {
                        root.controller.start();
                    }
                }
            }
        }

        QGCButton {
            enabled: root.controller && root.controller.hasRecording && !root.controller.recording && !root.controller.exporting
            objectName: "gpsRecordingExport"
            text: root.controller && root.controller.exporting ? qsTr("Exporting…") : qsTr("Export…")

            onClicked: {
                if (root.controller && root.controller.hasRecording && !root.controller.recording && !root.controller.exporting) {
                    exportDialog.openForSave();
                }
            }
        }
    }

    QGCButton {
        objectName: "gpsRecordingCancelExport"
        text: qsTr("Cancel export")
        visible: root.controller && root.controller.exporting
        onClicked: root.controller.cancelExport()
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsRecordingError"
        text: root.controller ? root.controller.errorString : ""
        visible: text.length > 0
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsRecordingExportPath"
        text: root.controller && root.controller.lastExportPath.length > 0 ? qsTr("Exported to %1").arg(root.controller.lastExportPath) : ""
        visible: text.length > 0
        wrapMode: Text.WrapAnywhere
    }

    QGCFileDialog {
        id: exportDialog

        defaultSuffix: "json"
        folder: root.exportFolder
        nameFilters: [qsTr("GPS recording (*.json)")]
        objectName: "gpsRecordingExportDialog"
        title: qsTr("Export GPS recording")

        onAcceptedForSave: file => {
            if (root.controller && !root.controller.recording && !root.controller.exporting) {
                root.controller.exportRecording(QGCFileDialogController.localFileToUrl(file));
            }
        }
    }
}
