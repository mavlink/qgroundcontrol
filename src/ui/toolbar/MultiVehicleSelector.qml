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
    visible:        _multiVehicles
    width:          _multiVehicles ? multiVehicleSelector.width : 0

    property bool _multiVehicles:  activeVehicle ? QGroundControl.multiVehicleManager.vehicles.count > 1 : false

    Connections {
        target:         QGroundControl.multiVehicleManager.vehicles
        onCountChanged: multiVehicleSelector.updatemultiVehiclesMenu()
    }

    QGCLabel {
        id:                     multiVehicleSelector
        text:                   "Vehicle " + (activeVehicle ? activeVehicle.id : "None")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.verticalCenter: parent.verticalCenter
        QGCMenu {
            id: multiVehiclesMenu
        }
        Component {
            id: multiVehicleMenuItemComponent
            QGCMenuItem {
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
                }
            }
        }
        Component.onCompleted: {
            multiVehicleSelector.updatemultiVehiclesMenu()
        }
    }
    MouseArea {
        visible:        _multiVehicles
        anchors.fill:   parent
        onClicked:      multiVehiclesMenu.popup()
    }
}
