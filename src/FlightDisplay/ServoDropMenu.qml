/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtLocation
import QtPositioning
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Vehicle
import QGroundControl.FlightMap

QGCPopupDialog {
    title:      qsTr("Servo Drop Control")
    buttons:    Dialog.Close

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _safetyOn:      true

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text: qsTr("Safety Switch:")
            }

            QGCSwitch {
                id: safetySwitch
                checked: _safetyOn
                onClicked: _safetyOn = checked
            }

            QGCLabel {
                text: _safetyOn ? qsTr("LOCKED") : qsTr("READY")
                color: _safetyOn ? "red" : "green"
                font.bold: true
            }
        }

        QGCButton {
            id:                 dropButton
            text:               qsTr("DROP")
            enabled:            !_safetyOn
            Layout.fillWidth:   true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            
            property var confirmDialog: null

            onClicked: {
                if (!confirmDialog) {
                    confirmDialog = dropConfirmation.createObject(mainWindow)
                }
                confirmDialog.open()
            }
        }
    }

    Component {
        id: dropConfirmation
        QGCSimpleMessageDialog {
            title:      qsTr("Confirm Drop")
            text:       qsTr("Are you sure you want to trigger the servo drop?")
            buttons:    Dialog.Yes | Dialog.No
            onAccepted: {
                // For now, we reuse the Gripper release command as a placeholder for "Drop"
                // This can be changed to a specific MAV_CMD_DO_SET_SERVO later if needed.
                _guidedController._gripperFunction = Vehicle.Gripper_release
                _activeVehicle.sendGripperAction(Vehicle.Gripper_release)
                _root.close()
            }
        }
    }
}
