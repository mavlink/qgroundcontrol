/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
