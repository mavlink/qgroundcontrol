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

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Mode Indicator
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          flightModeSelector.width

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _flightModes:      _activeVehicle ? _activeVehicle.flightModes : [ ]

    on_FlightModesChanged: flightModeSelector.updateFlightModesMenu()

    QGCLabel {
        id:                     flightModeSelector
        text:                   _activeVehicle ? _activeVehicle.flightMode : qsTr("N/A", "No data to display")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.verticalCenter: parent.verticalCenter
        Menu {
            id: flightModesMenu
        }
        Component {
            id: flightModeMenuItemComponent
            MenuItem {
                onTriggered: _activeVehicle.flightMode = text
            }
        }
        property var flightModesMenuItems: []
        function updateFlightModesMenu() {
            if (_activeVehicle && _activeVehicle.flightModeSetAvailable) {
                // Remove old menu items
                for (var i = 0; i < flightModesMenuItems.length; i++) {
                    flightModesMenu.removeItem(flightModesMenuItems[i])
                }
                flightModesMenuItems.length = 0
                // Add new items
                for (var i = 0; i < _flightModes.length; i++) {
                    var menuItem = flightModeMenuItemComponent.createObject(null, { "text": _flightModes[i] })
                    flightModesMenuItems.push(menuItem)
                    flightModesMenu.insertItem(i, menuItem)
                }
            }
        }
        Component.onCompleted: flightModeSelector.updateFlightModesMenu()
        MouseArea {
            visible:        _activeVehicle && _activeVehicle.flightModeSetAvailable
            anchors.fill:   parent
            onClicked:      flightModesMenu.popup()
        }
    }
}
