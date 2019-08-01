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

import QtQuick                              2.11
import QtQuick.Controls                     1.4

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Multi Vehicle Selector
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          selectorRow.width
    property bool _multiVehicles: QGroundControl.multiVehicleManager.vehicles.count > 1
    Component.onCompleted: {
        updatemultiVehiclesMenu()
    }
    Connections {
        target:         QGroundControl.multiVehicleManager.vehicles
        onCountChanged: updatemultiVehiclesMenu()
    }
    Row {
        id:                         selectorRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     parent.verticalCenter
        QGCColoredImage {
            anchors.verticalCenter: parent.verticalCenter
            height:                 ScreenTools.defaultFontPixelHeight
            width:                  height
            sourceSize.height:      parent.height
            fillMode:               Image.PreserveAspectFit
            source:                 "/qmlimages/PaperPlane.svg"
            color:                  qgcPal.text
        }
        QGCLabel {
            id:                     multiVehicleSelector
            text:                   "Vehicle " + (activeVehicle ? activeVehicle.id : "None")
            color:                  qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCColoredImage {
            visible:                _multiVehicles
            anchors.verticalCenter: parent.verticalCenter
            height:                 ScreenTools.defaultFontPixelHeight * 0.5
            width:                  height
            sourceSize.height:      parent.height
            fillMode:               Image.PreserveAspectFit
            source:                 "/res/DropArrow.svg"
            color:                  qgcPal.text
        }
    }
    Menu {
        id: multiVehiclesMenu
    }
    Component {
        id: multiVehicleMenuItemComponent
        MenuItem {
            onTriggered: QGroundControl.multiVehicleManager.activeVehicle = vehicle
            property int vehicleId: Number(text.split(" ")[1])
            property var vehicle:   QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
        }
    }
    property var multiVehiclesMenuItems: []
    function updatemultiVehiclesMenu() {
        if (_multiVehicles) {
            // Remove old menu items
            for (var i = 0; i < multiVehiclesMenuItems.length; i++) {
                multiVehiclesMenu.removeItem(multiVehiclesMenuItems[i])
            }
            multiVehiclesMenuItems.length = 0
            // Add new items
            for (i = 0; i < QGroundControl.multiVehicleManager.vehicles.count; i++) {
                var vehicle = QGroundControl.multiVehicleManager.vehicles.get(i)
                var menuItem = multiVehicleMenuItemComponent.createObject(null, { "text": "Vehicle " + vehicle.id })
                multiVehiclesMenuItems.push(menuItem)
                multiVehiclesMenu.insertItem(i, menuItem)
                console.log("Vehicle " + vehicle.id)
            }
        } else {
            console.log('No multiple vehicles: ' + QGroundControl.multiVehicleManager.vehicles.count)
        }
    }
    MouseArea {
        visible:        _multiVehicles
        anchors.fill:   parent
        onClicked: {
            console.log('Clicked')
            multiVehiclesMenu.popup()
        }
    }
}
