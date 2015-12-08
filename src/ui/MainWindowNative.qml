/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

QGROUNDCONTROL is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QGROUNDCONTROL is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

import QtQuick          2.5
import QtQuick.Window   2.2

import QGroundControl   1.0

/// Native QML top level window
Window {
    visible: true

    onClosing: {
        // Disallow window close if there are active connections
        if (QGroundControl.multiVehicleManager.activeVehicle) {
            showWindowCloseMessage()
            close.accepted = false
            return
        }

        // We still need to shutdown LinkManager even though no active connections so that we don't get any
        // more auto-connect links during shutdown.
        QGroundControl.linkManager.shutdown();
    }

    function showFlyView() {
        mainWindowInner.item.showFlyView()
    }

    function showPlanView() {
        mainWindowInner.item.showPlanView()
    }

    function showSetupView() {
        mainWindowInner.item.showSetupView()
    }

    function showWindowCloseMessage() {
        mainWindowInner.item.showWindowCloseMessage()
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
        mainWindowInner.showSetupVehicleComponent(vehicleComponent)
    }

    function showMessage(message) {
        mainWindowInner.item.showMessage(message)
    }

    Loader {
        id:             mainWindowInner
        anchors.fill:   parent
        source:         "MainWindowInner.qml"
    }
}

