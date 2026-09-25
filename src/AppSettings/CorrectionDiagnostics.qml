pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

SettingsGroupLayout {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections

    function destinationName(destinationId: string): string {
        if (destinationId === "udpOutput")
            return qsTr("UDP forwarding");
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
        text: root.corrections.hasSelectedStream
              ? qsTr("Current correction streams:")
              : qsTr("No fresh stream is selected for vehicles.")
        wrapMode: Text.WordWrap
    }

    Repeater {
        model: root.corrections.sourceInstances

        QGCLabel {
            required property gpsCorrectionStream modelData

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
                return qsTr("%1 — %2: %3").arg(root.corrections.sourceName(modelData.source))
                                         .arg(modelData.instanceId || qsTr("Default stream")).arg(status);
            }
            textFormat: Text.PlainText
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

        ColumnLayout {
            id: sourceRow

            required property int source
            required property double receivedBytesPerSecond
            required property double receivedFrames
            required property double validatedFrames
            required property double selectedFrames
            required property double queuedFrames
            required property double droppedFrames
            required property list<rtcmMessageCount> messageCounts

            Layout.fillWidth: true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
            spacing: ScreenTools.defaultFontPixelHeight / 4

            QGCLabel {
                Layout.fillWidth: true
                objectName: "correctionSource_" + sourceRow.source
                text: qsTr("%1 — received %2; frames: received %3, validated %4, selected %5, queued %6; drop events %7").arg(root.corrections.sourceName(sourceRow.source)).arg(GPSFormat.dataRate(sourceRow.receivedBytesPerSecond)).arg(sourceRow.receivedFrames).arg(sourceRow.validatedFrames).arg(sourceRow.selectedFrames).arg(sourceRow.queuedFrames).arg(sourceRow.droppedFrames)
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }

            RTCMMessageChips {
                Layout.fillWidth: true
                objectName: "correctionSourceMessages_" + sourceRow.source
                messageCounts: sourceRow.messageCounts
                visible: sourceRow.messageCounts.length > 0
            }
        }
    }

    Repeater {
        model: root.corrections.destinations

        QGCLabel {
            required property string destinationId
            required property double queuedBytes
            required property double droppedBytes

            Layout.fillWidth: true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
            objectName: "correctionDestination_" + destinationId
            text: qsTr("%1 — queued %2 B, dropped %3 B").arg(root.destinationName(destinationId)).arg(queuedBytes).arg(droppedBytes)
            textFormat: Text.PlainText
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

                text: qsTr("%1. %2 — %3, %4 B%5\nSource: %6 (session %7); destination: %8 (session %9)").arg(eventSequence).arg(root.corrections.sourceName(source)).arg(root.stageName(stage)).arg(bytes).arg(reason === GPSCorrectionEventModel.None ? "" : qsTr(" — %1").arg(root.reasonName(reason))).arg(sourceInstance || root.corrections.sourceName(source)).arg(sourceSession).arg(root.destinationName(destinationId)).arg(destinationSession)
                textFormat: Text.PlainText
                width: ListView.view.width
                wrapMode: Text.WordWrap
            }
        }
    }
}
