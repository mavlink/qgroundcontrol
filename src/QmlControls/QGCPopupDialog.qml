/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Dialogs  1.3

Item {
    property string title
    property var    buttons: StandardButton.Ok

    width:  childrenRect.width
    height: childrenRect.height

    signal hideDialog
    signal enableAcceptButton
    signal disableAcceptButton
    signal enableRejectButton
    signal disableRejectButton

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
            Qt.inputMethod.hide()
            hideDialog()
        }
    }

    function reject() {
        if (rejectAllowed) {
            Qt.inputMethod.hide()
            hideDialog()
        }
    }
}
