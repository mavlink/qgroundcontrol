/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.7
import QtQuick.Controls 2.1

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

// Label control whichs pop up a flight mode change menu when clicked
QGCLabel {
    id:     flightModeMenuLabel
    text:   activeVehicle ? activeVehicle.flightMode : qsTr("N/A", "No data to display")

    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Menu {
        id: flightModesMenu
    }

    Component {
        id: flightModeMenuItemComponent

        MenuItem {
            onTriggered: activeVehicle.flightMode = text
        }
    }

    property var flightModesMenuItems: []

    function updateFlightModesMenu() {
        if (activeVehicle && activeVehicle.flightModeSetAvailable) {
            // Remove old menu items
            for (var i = 0; i < flightModesMenuItems.length; i++) {
                flightModesMenu.removeItem(flightModesMenuItems[i])
            }
            flightModesMenuItems.length = 0
            // Add new items
            for (var i = 0; i < activeVehicle.flightModes.length; i++) {
                var menuItem = flightModeMenuItemComponent.createObject(null, { "text": activeVehicle.flightModes[i] })
                flightModesMenuItems.push(menuItem)
                flightModesMenu.insertItem(i, menuItem)
            }
        }
    }

    Component.onCompleted: flightModeMenuLabel.updateFlightModesMenu()

    Connections {
        target:                 QGroundControl.multiVehicleManager
        onActiveVehicleChanged: flightModeMenuLabel.updateFlightModesMenu()
    }

    MouseArea {
        visible:        activeVehicle && activeVehicle.flightModeSetAvailable
        anchors.fill:   parent
        onClicked:      flightModesMenu.popup()
    }
}
