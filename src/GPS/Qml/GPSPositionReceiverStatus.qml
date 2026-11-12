pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.GPS

/// The position receiver's connection, with Connect and its problem, and its live status: GPSReceiverStatus for the
/// second receiver, where the GNSS receiver's is shown. It exists only while the position receiver is in use, so a
/// view that shows both receivers has the rows and Connect of one receiver until then.
Loader {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.positionReceiver
    property PositionReceiverSettings settings: QGroundControl.settingsManager.positionReceiverSettings
    property string heading: qsTr("Position Receiver Status")
    property bool showBorder: true

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    active: root.settings.enabled.rawValue
    visible: active

    sourceComponent: GPSReceiverStatus {
        objectName: "positionReceiverStatus"
        receiver: root.receiver
        facts: root.receiver.facts
        heading: root.heading
        showBorder: root.showBorder
        showWhenDisconnected: true
        disconnectedText: qsTr("No position receiver connected.")
    }
}
