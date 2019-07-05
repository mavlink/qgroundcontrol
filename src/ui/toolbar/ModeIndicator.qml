/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                              2.11
import QtQuick.Controls                     2.4

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

    property var _flightModes:      activeVehicle ? activeVehicle.flightModes : [ ]

    on_FlightModesChanged: flightModeSelector.updateFlightModesMenu()

    QGCLabel {
        id:                     flightModeSelector
        text:                   activeVehicle ? activeVehicle.flightMode : qsTr("N/A", "No data to display")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.verticalCenter: parent.verticalCenter
        QGCMenu {
            id: flightModesMenu
        }
        Component {
            id: flightModeMenuItemComponent
            QGCMenuItem {
                onTriggered: activeVehicle.flightMode = text
            }
        }
        property var flightModesMenuItems: []
        function updateFlightModesMenu() {
            if (activeVehicle && activeVehicle.flightModeSetAvailable) {
                // Remove old menu items
                var i
                for (i = 0; i < flightModesMenuItems.length; i++) {
                    flightModesMenu.removeItem(flightModesMenuItems[i])
                }
                flightModesMenuItems.length = 0
                // Add new items
                for (i = 0; i < _flightModes.length; i++) {
                    var menuItem = flightModeMenuItemComponent.createObject(null, { "text": _flightModes[i] })
                    flightModesMenuItems.push(menuItem)
                    flightModesMenu.insertItem(i, menuItem)
                }
            }
        }
        Component.onCompleted: flightModeSelector.updateFlightModesMenu()
        MouseArea {
            visible:        activeVehicle && activeVehicle.flightModeSetAvailable
            anchors.fill:   parent
            onClicked:      flightModesMenu.popup()
        }
    }
}
