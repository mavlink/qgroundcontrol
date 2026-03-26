import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("Link Status (Current Vehicle)")

    property var  _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property string _notConnectedStr: qsTr("Not Connected")

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Total messages sent (computed)")
        labelText:          _activeVehicle ? _activeVehicle.mavlinkSentCount : _notConnectedStr
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Total messages received")
        labelText:          _activeVehicle ? _activeVehicle.mavlinkReceivedCount : _notConnectedStr
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Total message loss")
        labelText:          _activeVehicle ? _activeVehicle.mavlinkLossCount : _notConnectedStr
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Loss rate")
        labelText:          _activeVehicle ? _activeVehicle.mavlinkLossPercent.toFixed(0) + '%' : _notConnectedStr
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Signing")
        labelText:          _activeVehicle ? (_activeVehicle.mavlinkSigning ? qsTr("On") : qsTr("Off")) : _notConnectedStr
    }
}
