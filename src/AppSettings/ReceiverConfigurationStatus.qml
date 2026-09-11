import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    property var report: QGroundControl.gpsManager.configurationReport
    property bool reportActive: QGroundControl.gpsManager.configurationReportActive

    function formatValue(value, units) {
        return value === undefined || value === null ? qsTr("Unknown") : units ? qsTr("%1 %2").arg(value).arg(units) : String(value);
    }

    heading: qsTr("Receiver Configuration Status")

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsConfigurationReportSummary"
        text: !root.report || root.report.length === 0 ? root.reportActive ? qsTr("No setting confirmation is available for this connection.") : qsTr("No configuration report. Connect a receiver to obtain one.") : root.reportActive ? qsTr("Current connection. Acknowledgement confirms a request was accepted; only readback confirms the receiver's reported value.") : qsTr("Previous connection report. These values do not describe a currently connected receiver.")
        wrapMode: Text.WordWrap
    }

    QGCCheckBox {
        id: showDetails

        enabled: root.report && root.report.length > 0
        objectName: "gpsConfigurationReportToggle"
        text: qsTr("Show requested and reported settings")
    }

    Loader {
        Layout.fillWidth: true
        active: showDetails.checked && showDetails.enabled
        visible: active

        sourceComponent: ColumnLayout {
            objectName: "gpsConfigurationReportDetails"
            spacing: ScreenTools.defaultFontPixelHeight

            Repeater {
                model: root.report

                ColumnLayout {
                    id: setting

                    required property var modelData

                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelHeight / 4

                    QGCLabel {
                        Layout.fillWidth: true
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                        text: qsTr("%1 — requested: %2").arg(setting.modelData.label || setting.modelData.key).arg(root.formatValue(setting.modelData.requestedValue, setting.modelData.units || ""))
                        wrapMode: Text.WordWrap
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        objectName: "gpsConfigurationRequest_" + setting.modelData.key
                        text: setting.modelData.requestState === 1 ? qsTr("Request acknowledged") : setting.modelData.requestState === 2 ? qsTr("Request rejected") : qsTr("Requested; not acknowledged")
                        wrapMode: Text.WordWrap
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        objectName: "gpsConfigurationReadback_" + setting.modelData.key
                        text: setting.modelData.readbackState !== 1 ? qsTr("Receiver readback unavailable") : setting.modelData.comparisonApplicable && !setting.modelData.matchesRequested ? qsTr("Reported: %1 — differs from the request").arg(root.formatValue(setting.modelData.reportedValue, setting.modelData.units || "")) : qsTr("Reported: %1").arg(root.formatValue(setting.modelData.reportedValue, setting.modelData.units || ""))
                        wrapMode: Text.WordWrap
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                        text: setting.modelData.detail || ""
                        visible: text.length > 0
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
