pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.GPS

/// How the position a fixed base was given compares with the one its receiver found on its own before, for the
/// receiver families that check: the distance between the two, or that the receiver had no position to compare. A
/// distance too large for one place also shows as the receiver's problem, with what to do about it.
GPSWrappingRow {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver

    readonly property int _check: root.receiver.fixedBaseCheck
    //: %1 is a distance with its unit, such as 3 m or 12.4 km
    readonly property string _apart: qsTr("%1 from the receiver's own position")
                                         .arg(GPSUnits.distanceText(root.receiver.fixedBaseCheckDistance))

    objectName: "rtkFixedBaseCheck"
    visible: root.receiver.state === GPSReceiver.Connected
             && (root._check === GPSReceiver.Unchecked || root._check === GPSReceiver.Plausible
                 || root._check === GPSReceiver.Implausible)
    //: Label of the comparison of a fixed base position with the position the receiver found on its own
    label: qsTr("Position check")
    labelText: root._check === GPSReceiver.Unchecked ? qsTr("Not checked: the receiver found no position of its own")
                                                     : root._apart
    labelWrapMode: Text.Wrap
}
