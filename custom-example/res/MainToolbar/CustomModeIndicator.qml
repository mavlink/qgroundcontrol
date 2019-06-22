/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick          2.11
import QtQuick.Controls 1.4

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Mode Indicator
Item {
    anchors.top:                    parent.top
    anchors.bottom:                 parent.bottom
    width:                          selectorRow.width

    property var _flightModes:      activeVehicle ? activeVehicle.flightModes : [ ]

    on_FlightModesChanged:          flightModeSelector.updateFlightModesMenu()

    Row {
        id:                         selectorRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     parent.verticalCenter
        QGCLabel {
            id:                     flightModeSelector
            text:                   activeVehicle ? activeVehicle.flightMode : qsTr("N/A")
            color:                  qgcPal.text
            font.pointSize:         ScreenTools.defaultFontPointSize
            anchors.verticalCenter:     parent.verticalCenter
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
                    var i = 0
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
        }
        QGCColoredImage {
            anchors.verticalCenter: parent.verticalCenter
            height:                 ScreenTools.defaultFontPixelHeight * 0.5
            width:                  height
            sourceSize.height:      parent.height
            fillMode:               Image.PreserveAspectFit
            source:                 "/res/DropArrow.svg"
            color:                  qgcPal.text
        }
    }
    Component.onCompleted: flightModeSelector.updateFlightModesMenu()
    MouseArea {
        visible:        activeVehicle && activeVehicle.flightModeSetAvailable
        anchors.fill:   parent
        onClicked:      flightModesMenu.popup()
    }
}
