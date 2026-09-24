pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections

    function destinationName(destinationId: string): string {
        if (destinationId === "ntripUdp")
            return qsTr("NTRIP UDP output");
        if (destinationId === "mavlink")
            return qsTr("Vehicles");
        if (destinationId.startsWith("mavlink/"))
            return qsTr("Vehicle link %1").arg(destinationId.slice(8));
        return destinationId || qsTr("Unselected");
    }

    function reasonName(reason) {
        switch (reason) {
        case GPSCorrectionEventModel.None:
            return "";
        case GPSCorrectionEventModel.InvalidTimestamp:
            return qsTr("Invalid receipt time");
        case GPSCorrectionEventModel.Expired:
            return qsTr("Expired");
        case GPSCorrectionEventModel.MessageFiltered:
            return qsTr("Filtered message");
        case GPSCorrectionEventModel.NotSelected:
            return qsTr("Source not selected");
        case GPSCorrectionEventModel.DestinationUnavailable:
            return qsTr("Destination unavailable");
        case GPSCorrectionEventModel.InvalidFrame:
            return qsTr("Invalid frame");
        default:
            return qsTr("Unknown");
        }
    }

    function sourceName(source) {
        switch (source) {
        case GPSCorrectionSettings.LocalReceiver:
            return qsTr("Local base station");
        case GPSCorrectionSettings.Ntrip:
            return qsTr("NTRIP");
        case GPSCorrectionSettings.Udp:
            return qsTr("UDP");
        default:
            return qsTr("Unclassified");
        }
    }

    function dataRate(bytesPerSecond) {
        //: Data rate in bytes per second
        if (bytesPerSecond < 1024) return qsTr("%1 B/s").arg(bytesPerSecond)
        //: Data rate in kilobytes per second
        return qsTr("%1 KB/s").arg((bytesPerSecond / 1024).toFixed(1))
    }

    function stageName(stage) {
        switch (stage) {
        case GPSCorrectionEventModel.Received:
            return qsTr("Received");
        case GPSCorrectionEventModel.Validated:
            return qsTr("Validated");
        case GPSCorrectionEventModel.Selected:
            return qsTr("Selected");
        case GPSCorrectionEventModel.Queued:
            return qsTr("Queued");
        case GPSCorrectionEventModel.Dropped:
            return qsTr("Dropped");
        default:
            return qsTr("Unknown");
        }
    }

    heading: qsTr("Correction Diagnostics")
    objectName: "correctionDiagnostics"

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "correctionSelectionStatus"
        text: root.corrections.sourceInstances.some(source => source.selected)
              ? qsTr("Current correction streams:")
              : qsTr("No fresh stream is selected for vehicles.")
        wrapMode: Text.WordWrap
    }

    Repeater {
        model: root.corrections.sourceInstances

        QGCLabel {
            required property var modelData

            Layout.fillWidth: true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
            objectName: "correctionStreamState_" + modelData.source + "_" + modelData.instanceId
            text: {
                let status;
                if (modelData.selected)
                    status = qsTr("Selected for vehicles; fresh");
                else if (modelData.usable)
                    status = qsTr("Fresh; not selected for vehicles");
                else if (modelData.active)
                    status = qsTr("Active; waiting for fresh corrections");
                else
                    status = qsTr("Unavailable");
                return qsTr("%1 — %2: %3").arg(root.sourceName(modelData.source))
                                         .arg(modelData.instanceId || qsTr("Default stream")).arg(status);
            }
            wrapMode: Text.WordWrap
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        text: qsTr("Queued bytes have been admitted to an output. They do not confirm that a receiver applied the corrections or obtained a fix.")
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        text: qsTr("Received rates measure frame-candidate bytes, not raw transport traffic. Recovered frames can overlap rejected candidates. Drop events count separate selection and admission losses; one frame may contribute more than once.")
        wrapMode: Text.WordWrap
    }

    Repeater {
        model: root.corrections.sources

        QGCLabel {
            required property var modelData

            Layout.fillWidth: true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
            objectName: "correctionSource_" + modelData.source
            text: qsTr("%1 — received %2; frames: received %3, validated %4, selected %5, queued %6; drop events %7").arg(root.sourceName(modelData.source)).arg(root.dataRate(modelData.receivedBytesPerSecond)).arg(modelData.receivedFrames).arg(modelData.validatedFrames).arg(modelData.selectedFrames).arg(modelData.queuedFrames).arg(modelData.droppedFrames)
            wrapMode: Text.WordWrap
        }
    }

    Repeater {
        model: root.corrections.destinations

        QGCLabel {
            required property var modelData

            Layout.fillWidth: true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
            objectName: "correctionDestination_" + modelData.destinationId
            text: qsTr("%1 — queued %2 B, dropped %3 B").arg(root.destinationName(modelData.destinationId)).arg(modelData.queuedBytes).arg(modelData.droppedBytes)
            wrapMode: Text.WordWrap
        }
    }

    QGCCheckBox {
        id: historyToggle

        objectName: "correctionHistoryToggle"
        text: qsTr("Show recent correction events")
    }

    Loader {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        active: historyToggle.checked
        visible: active

        sourceComponent: ListView {
            clip: true
            implicitHeight: ScreenTools.defaultFontPixelHeight * 16
            model: root.corrections.events
            objectName: "correctionEventHistory"
            reuseItems: true
            spacing: ScreenTools.defaultFontPixelHeight / 2

            delegate: QGCLabel {
                required property double bytes
                required property string destinationId
                required property double destinationSession
                required property double eventSequence
                required property int reason
                required property int source
                required property string sourceInstance
                required property double sourceSession
                required property int stage

                text: qsTr("%1. %2 — %3, %4 B%5\nSource: %6 (session %7); destination: %8 (session %9)").arg(eventSequence).arg(root.sourceName(source)).arg(root.stageName(stage)).arg(bytes).arg(reason === GPSCorrectionEventModel.None ? "" : qsTr(" — %1").arg(root.reasonName(reason))).arg(sourceInstance || root.sourceName(source)).arg(sourceSession).arg(root.destinationName(destinationId)).arg(destinationSession)
                width: ListView.view.width
                wrapMode: Text.WordWrap
            }
        }
    }
}
