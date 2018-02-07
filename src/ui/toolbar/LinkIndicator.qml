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
//-- Link Indicator
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          priorityLinkSelector.width

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCLabel {
        id:                     priorityLinkSelector
        text:                   _activeVehicle ? _activeVehicle.priorityLinkName : qsTr("N/A", "No data to display")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.verticalCenter: parent.verticalCenter
        Menu {
            id: linkSelectionMenu
        }
        Component {
            id: linkSelectionMenuItemComponent
            MenuItem {
                onTriggered: _activeVehicle.priorityLinkName = text
            }
        }
        property var linkSelectionMenuItems: []
        function updatelinkSelectionMenu() {
            if (_activeVehicle) {
                // Remove old menu items
                for (var i = 0; i < linkSelectionMenuItems.length; i++) {
                    linkSelectionMenu.removeItem(linkSelectionMenuItems[i])
                }
                linkSelectionMenuItems.length = 0
                // Add new items
                for (var i = 0; i < _activeVehicle.linkNames.length; i++) {
                    var menuItem = linkSelectionMenuItemComponent.createObject(null, { "text": _activeVehicle.linkNames[i] })
                    linkSelectionMenuItems.push(menuItem)
                    linkSelectionMenu.insertItem(i, menuItem)
                }
            }
        }
        Component.onCompleted: priorityLinkSelector.updatelinkSelectionMenu()
        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: priorityLinkSelector.updatelinkSelectionMenu()
        }
        Connections {
            target:                 _activeVehicle
            onLinkNamesChanged:     priorityLinkSelector.updatelinkSelectionMenu()
        }
        MouseArea {
            visible:        _activeVehicle
            anchors.fill:   parent
            onClicked:      linkSelectionMenu.popup()
        }
    }
}
