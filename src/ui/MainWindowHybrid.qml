/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl           1.0
import QGroundControl.Controls  1.0

/// Native QML top level window
Item {
    function showFlyView() {
        mainWindowInner.item.showFlyView()
    }

    function showPlanView() {
        mainWindowInner.item.showPlanView()
    }

    function showSetupView() {
        mainWindowInner.item.showSetupView()
    }

    function attemptWindowClose() {
        mainWindowInner.item.attemptWindowClose()
    }

    // The following are use for unit testing only

    function showSetupFirmware() {
        mainWindowInner.item.showSetupFirmware()
    }

    function showSetupParameters() {
        mainWindowInner.item.showSetupParameters()
    }

    function showSetupSummary() {
        mainWindowInner.item.showSetupSummary()
    }

    function showSetupVehicleComponent(vehicleComponent) {
        mainWindowInner.item.showSetupVehicleComponent(vehicleComponent)
    }

    function showMessage(message) {
        mainWindowInner.item.showMessage(message)
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
