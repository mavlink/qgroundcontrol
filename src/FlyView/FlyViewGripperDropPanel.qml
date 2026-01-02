import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: ScreenTools.defaultFontHeight / 2

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _buttonTitles: [qsTr("Open"), qsTr("Close"), qsTr("Stop")]
    property var _buttonActions: [QGCMAVLink.GripperActionOpen, QGCMAVLink.GripperActionClose, QGCMAVLink.GripperActionStop]

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
