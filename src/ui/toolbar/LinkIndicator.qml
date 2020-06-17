/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
import QGroundControl.Vehicle               1.0

//-------------------------------------------------------------------------
//-- Link Indicator
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          priorityLinkSelector.width

    property bool showIndicator: false

    QGCLabel {
        id:                     priorityLinkSelector
        text:                   activeVehicle ? activeVehicle.priorityLinkName : qsTr("N/A", "No data to display")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.verticalCenter: parent.verticalCenter
        QGCMenu {
            id: linkSelectionMenu
        }
        Component {
            id: linkSelectionMenuItemComponent
            QGCMenuItem {
                onTriggered: activeVehicle.priorityLinkName = text
            }
        }
        property var linkSelectionMenuItems: []
        function updatelinkSelectionMenu() {
            if (activeVehicle) {
                // Remove old menu items
                var i
                for (i = 0; i < linkSelectionMenuItems.length; i++) {
                    linkSelectionMenu.removeItem(linkSelectionMenuItems[i])
                }
                linkSelectionMenuItems.length = 0

                // Add new items
                var has_hl = false;
                var links = activeVehicle.links
                for (i = 0; i < links.length; i++) {
                    var menuItem = linkSelectionMenuItemComponent.createObject(null, { "text": links[i].getName(), "enabled": links[i].link_active(activeVehicle.id)})
                    linkSelectionMenuItems.push(menuItem)
                    linkSelectionMenu.insertItem(i, menuItem)

                    if (links[i].getHighLatency()) {
                        has_hl = true
                    }
                }

                showIndicator = links.length > 1 && has_hl
            }
        }

        Component.onCompleted: priorityLinkSelector.updatelinkSelectionMenu()

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: priorityLinkSelector.updatelinkSelectionMenu()
        }

        Connections {
            target:                 activeVehicle
            onLinksChanged:         priorityLinkSelector.updatelinkSelectionMenu()
        }

        Connections {
            target:                     activeVehicle
            onLinksPropertiesChanged:   priorityLinkSelector.updatelinkSelectionMenu()
        }

        MouseArea {
            visible:        activeVehicle
            anchors.fill:   parent
            onClicked:      linkSelectionMenu.popup()
        }
    }
}
