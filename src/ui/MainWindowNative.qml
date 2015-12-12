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
import QtQuick.Dialogs  1.2

import QGroundControl   1.0

/// Native QML top level window
Window {
    id:         _rootWindow
    visible:    true

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
        windowCloseDialog.open()
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

    MessageDialog {
        id:                 windowCloseDialog
        title:              "QGroundControl close"
        text:               "There are still active connections to vehicles. Do you want to disconnect these before closing?"
        standardButtons:    StandardButton.Yes | StandardButton.Cancel
        modality:           Qt.ApplicationModal
        visible:            false

        onYes: {
            QGroundControl.linkManager.shutdown()
            // The above shutdown causes a flurry of activity as the vehicle components are removed. This in turn
            // causes the Windows Version of Qt to crash if you allow the close event to be accepted. In order to prevent
            // the crash, we ignore the close event and setup a delayed timer to close the window after things settle down.
            delayedWindowCloseTimer.start()
        }
    }

    Timer {
        id:         delayedWindowCloseTimer
        interval:   1500
        running:    false
        repeat:     false

        onTriggered: _rootWindow.close()
    }

}

