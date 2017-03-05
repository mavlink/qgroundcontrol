/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

FactPanel {
    property var qgcTextFieldforwardKeysTo: this    ///< Causes all QGCTextFields to forward keys here if they have focus

    QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }

    signal hideDialog

    Keys.onReleased: {
        if (event.key == Qt.Key_Escape) {
            reject()
            event.accepted = true
        } else if (event.key == Qt.Key_Return) {
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

    color: __qgcPal.windowShadeDark
}
