pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// What the GNSS receiver is used as: one choice over the two settings that say it together, the receiver's role and
/// whether the RTCM of a receiver that is not configured is forwarded to vehicles.
GPSField {
    id: root

    required property RTKSettings settings
    property bool editable: true

    /// The uses, in the order they are offered.
    enum Use {
        RtkBase,
        ExistingBase,
        PositionOnly
    }

    /// The use the saved settings describe. Only a configured base is configured, so any other role is that of a
    /// receiver that is listened to, and a configured base's corrections are forwarded whatever the second setting
    /// holds: every pair of saved values is one of the three.
    readonly property int use: settings.receiverRole.rawValue === RTKSettings.ConfiguredBase
                               ? GPSReceiverUse.RtkBase
                               : settings.forwardReceiverRtcm.rawValue ? GPSReceiverUse.ExistingBase
                                                                       : GPSReceiverUse.PositionOnly

    readonly property string _unconfigured: qsTr("Its NMEA, u-blox UBX or Septentrio SBF position output provides the ground station position. Configure the receiver's output externally and select its baud rate, or Auto to find the rate by listening, which takes up to 20 seconds. No survey-in status is inferred.")

    function _select(use) {
        if (use === GPSReceiverUse.RtkBase) {
            root.settings.receiverRole.rawValue = RTKSettings.ConfiguredBase
            return
        }
        // Forwarding first: the role is what makes the receiver follow it.
        root.settings.forwardReceiverRtcm.rawValue = use === GPSReceiverUse.ExistingBase
        root.settings.receiverRole.rawValue = RTKSettings.Passive
    }

    label: qsTr("Use receiver as")
    note: {
        switch (root.use) {
        case GPSReceiverUse.RtkBase:
            return qsTr("QGroundControl configures a supported receiver as an RTK base station and forwards its RTCM corrections to vehicles.")
        case GPSReceiverUse.ExistingBase:
            return qsTr("The receiver is a base station already. QGroundControl never configures it, and forwards the RTCM corrections it sends to vehicles.")
                   + " " + root._unconfigured
        default:
            return qsTr("QGroundControl never configures this receiver, and forwards none of its output to vehicles.")
                   + " " + root._unconfigured
        }
    }

    QGCComboBox {
        objectName: "rtkReceiverUse"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        enabled: root.editable
        model: [qsTr("RTK base station"), qsTr("Existing base station"), qsTr("Position only")]
        currentIndex: root.use
        onActivated: index => root._select(index)
    }
}
