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
import QGroundControl.Vehicle               1.0

//-------------------------------------------------------------------------
//-- Link Indicator
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          visible ? priorityLinkSelector.width : 0
    visible:        _visible

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _visible: false

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
                var has_hl = false;
                var links = _activeVehicle.links
                for (var i = 0; i < links.length; i++) {
                    var menuItem = linkSelectionMenuItemComponent.createObject(null, { "text": links[i].getName(), "enabled": links[i].link_active(_activeVehicle.id)})
                    linkSelectionMenuItems.push(menuItem)
                    linkSelectionMenu.insertItem(i, menuItem)

                    if (links[i].getHighLatency()) {
                        has_hl = true
                    }
                }

                _visible = links.length > 1 && has_hl
            }
        }

        Component.onCompleted: priorityLinkSelector.updatelinkSelectionMenu()

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: priorityLinkSelector.updatelinkSelectionMenu()
        }

        Connections {
            target:                 _activeVehicle
            onLinksChanged:         priorityLinkSelector.updatelinkSelectionMenu()
        }

        Connections {
            target:                     _activeVehicle
            onLinksPropertiesChanged:   priorityLinkSelector.updatelinkSelectionMenu()
        }

        MouseArea {
            visible:        _activeVehicle
            anchors.fill:   parent
            onClicked:      linkSelectionMenu.popup()
        }
    }
}
