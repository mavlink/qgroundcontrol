import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The correction stream vehicles receive and the NTRIP caster connection, shared by the GPS indicator and the
/// RTK Corrections settings page.
SettingsGroupLayout {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections
    property NTRIPManager ntrip: QGroundControl.gpsManager.ntrip
    /// Keep the group visible while no correction source is configured.
    property bool showWhenInactive: true

    readonly property gpsCorrectionStream _selected: root.corrections.selectedStream
    readonly property int _ntripStatus: root.ntrip.connectionStatus
    readonly property bool _ntripActive: root._ntripStatus !== NTRIPManager.Disconnected
    readonly property bool _active: root._ntripActive || root.corrections.sourceInstances.length > 0

    heading: qsTr("Corrections")
    visible: root.showWhenInactive || root._active

    LabelledLabel {
        objectName: "correctionsSelectedSource"
        Layout.fillWidth: true
        label: qsTr("Source")
        labelText: root._selected.selected ? root.corrections.sourceName(root._selected.source)
                   //: No fresh correction stream is available yet
                   : root._active ? qsTr("Waiting") : qsTr("None")
    }

    // Stream endpoints can be long URLs, so they wrap rather than widen the group.
    QGCLabel {
        objectName: "correctionsSelectedStream"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        visible: root._selected.selected && root._selected.instanceId.length > 0
        text: root._selected.instanceId
        textFormat: Text.PlainText
        wrapMode: Text.WrapAnywhere
        font.pointSize: ScreenTools.smallFontPointSize
    }

    LabelledLabel {
        objectName: "correctionsDataRate"
        Layout.fillWidth: true
        visible: root._selected.selected
        label: qsTr("Data rate")
        labelText: GPSFormat.dataRate(root.corrections.selectedBytesPerSecond)
    }

    LabelledLabel {
        objectName: "correctionsNtripStatus"
        Layout.fillWidth: true
        visible: root._ntripActive
        label: qsTr("NTRIP")
        labelText: {
            switch (root._ntripStatus) {
            case NTRIPManager.Connecting: return qsTr("Connecting")
            case NTRIPManager.Connected: return qsTr("Connected")
            case NTRIPManager.Reconnecting: return qsTr("Reconnecting")
            default: return qsTr("Error")
            }
        }
    }

    QGCLabel {
        objectName: "correctionsNtripError"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        visible: root._ntripStatus === NTRIPManager.Error && root.ntrip.statusMessage.length > 0
        text: root.ntrip.statusMessage
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
    }
}
