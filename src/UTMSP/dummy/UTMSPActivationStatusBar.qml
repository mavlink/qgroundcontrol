import QtQuick 2.3
import QGroundControl.UTMSP                 1.0

Item {
    property string activationStartTimestamp
    property bool   activationApproval
    property string flightID
    property string timeDifference

    signal  activationTriggered(bool value)
}
