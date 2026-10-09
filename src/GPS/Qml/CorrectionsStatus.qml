pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The correction stream vehicles receive, shared by the GPS indicator and the RTK Corrections settings page.
SettingsGroupLayout {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections
    /// Keep the group visible while no correction source is configured.
    property bool showWhenInactive: true

    readonly property gpsCorrectionStream _selected: root.corrections.selectedStream
    readonly property bool _active: root.corrections.state !== GPSCorrectionManager.Inactive
    readonly property bool _fresh: root.corrections.state === GPSCorrectionManager.Fresh
    readonly property string _udpInputError: root.corrections.udpInputError
    readonly property string _udpOutputError: root.corrections.udpOutputError

    heading: qsTr("Corrections Status")
    visible: root.showWhenInactive || root._active || root._udpInputError !== "" || root._udpOutputError !== ""

    LabelledLabel {
        objectName: "correctionsSelectedSource"
        label: qsTr("Source")
        labelText: root._fresh ? root.corrections.sourceName(root._selected.source)
                   //: No fresh correction stream is available yet
                   : root._active ? qsTr("Waiting") : qsTr("None")
    }

    // Stream endpoints can be long URLs, so they wrap anywhere.
    GPSNoteLabel {
        objectName: "correctionsSelectedStream"
        visible: root._fresh && root._selected.instanceId.length > 0
        text: root._selected.instanceId
        wrapMode: Text.WrapAnywhere
    }

    LabelledLabel {
        objectName: "correctionsDataRate"
        visible: root._fresh
        label: qsTr("Data rate")
        //: %1 is a data size, such as 2.1KB; the label is a data rate per second
        labelText: qsTr("%1/s").arg(QGroundControl.bigSizeToString(root.corrections.selectedBytesPerSecond))
    }

    // Why the UDP RTCM input cannot listen, such as a port another program uses; the input keeps retrying.
    GPSNoteLabel {
        objectName: "correctionsUdpInputError"
        visible: root._udpInputError !== ""
        text: root._udpInputError
        warning: true
    }

    // Why the enabled UDP output forwards nothing, such as an invalid address or a loop into the UDP input.
    GPSNoteLabel {
        objectName: "correctionsUdpOutputError"
        visible: root._udpOutputError !== ""
        text: root._udpOutputError
        warning: true
    }
}
