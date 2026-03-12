import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: ScreenTools.defaultFontHeight / 2

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _buttonTitles: [qsTr("Release"), qsTr("Grab"), qsTr("Hold")]
    property var _buttonActions: [QGCMAVLink.GripperActionRelease, QGCMAVLink.GripperActionGrab, QGCMAVLink.GripperActionHold]

    Repeater {
        model: _buttonTitles

        QGCDelayButton {
            Layout.fillWidth:   true
            text:               _buttonTitles[index]

            onActivated: {
                _activeVehicle.sendGripperAction(_buttonActions[index])
                dropPanel.hide()
            }
        }
    }
}
