/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.3

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

Item {
    property var buttons: StandardButton.NoButton

    signal hideDialog

    Keys.onReleased: {
        if (event.key === Qt.Key_Escape) {
            reject()
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            accept()
            event.accepted = true
        }
    }

    function accept() {
        if (acceptAllowed) {
            hideDialog()
        }
    }

    function reject() {
        if (rejectAllowed) {
            hideDialog()
        }
    }
}
