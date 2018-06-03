/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl           1.0
import QGroundControl.Controls  1.0

/// Native QML top level window
Item {
    function showSetupView() {
        mainWindowInner.item.showSetupView()
    }

    function attemptWindowClose() {
        if(!mainWindowInner.item) {
            controller.reallyClose()
        } else {
            mainWindowInner.item.attemptWindowClose()
        }
    }

    function showMessage(message) {
        if(mainWindowInner.item) {
            mainWindowInner.item.showMessage(message)
        } else {
            console.log(message)
        }
    }

    Loader {
        id:             mainWindowInner
        anchors.fill:   parent
        source:         "MainWindowInner.qml"

        Connections {
            target: mainWindowInner.item

            onReallyClose: controller.reallyClose()
        }
    }
}
