import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    property var corrections: QGroundControl.gpsManager.corrections

    function destinationName(destinationId: string): string {
        if (destinationId === "localReceiver")
            return qsTr("Local receiver");
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
        case GPSCorrectionEventModel.InactiveSource:
            return qsTr("Inactive source");
        case GPSCorrectionEventModel.SessionMismatch:
            return qsTr("Previous source session");
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
        case GPSCorrectionEventModel.QueueFull:
            return qsTr("Queue full");
        case GPSCorrectionEventModel.InvalidFrame:
            return qsTr("Invalid frame");
        case GPSCorrectionEventModel.Cancelled:
            return qsTr("Connection cancelled");
        case GPSCorrectionEventModel.SourceChanged:
            return qsTr("Source changed");
        case GPSCorrectionEventModel.WriteFailed:
            return qsTr("Write failed");
        case GPSCorrectionEventModel.PartialWrite:
            return qsTr("Incomplete write");
        case GPSCorrectionEventModel.InvalidDelivery:
            return qsTr("Unmatched delivery report");
        case GPSCorrectionEventModel.DeliveryUnconfirmed:
            return qsTr("Connection ended before the write result was available");
        case GPSCorrectionEventModel.DiagnosticsBackpressure:
            return qsTr("Delivery tracking full");
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
        case GPSCorrectionEventModel.Written:
            return qsTr("Written");
        case GPSCorrectionEventModel.Unconfirmed:
            return qsTr("Unconfirmed");
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
        text: qsTr("Queued bytes have been submitted to an output. Written bytes reached the local receiver's transport; they do not confirm that the receiver applied the corrections. Vehicle writes are unconfirmed.")
        wrapMode: Text.WordWrap
    }

    Repeater {
        model: root.corrections.sources

        QGCLabel {
            required property var modelData

            Layout.fillWidth: true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
            text: qsTr("%1 — frames: received %2, validated %3, selected %4, queued %5, written %6, dropped %7").arg(root.sourceName(modelData.source)).arg(modelData.receivedFrames).arg(modelData.validatedFrames).arg(modelData.selectedFrames).arg(modelData.queuedFrames).arg(modelData.writtenFrames).arg(modelData.droppedFrames)
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
            text: qsTr("%1 — queued %2 B, written %3, dropped %4 B, pending %5 B, unconfirmed %6 B").arg(root.destinationName(modelData.destinationId)).arg(modelData.queuedBytes).arg(modelData.reportsWrites ? qsTr("%1 B").arg(modelData.writtenBytes) : qsTr("unconfirmed")).arg(modelData.droppedBytes).arg(modelData.pendingBytes).arg(modelData.unconfirmedBytes)
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
