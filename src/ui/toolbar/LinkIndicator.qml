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

Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          primaryLinkSelector.width

    property bool showIndicator: false

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _rgLinkNames:      _activeVehicle ? _activeVehicle.vehicleLinkManager.linkNames : [ ]
    property var _rgLinkStatus:     _activeVehicle ? _activeVehicle.vehicleLinkManager.linkStatuses : [ ]
    property var _rgMenuItems:      [ ]

    function updateLinkSelectionMenu() {
        // Remove old menu items
        var i
        for (i = 0; i < _rgMenuItems.length; i++) {
            linkSelectionMenu.removeItem(_rgMenuItems[i])
        }
        _rgMenuItems.length = 0

        // Add new items
        for (i = 0; i < _rgLinkNames.length; i++) {
            var menuItem = linkSelectionMenuItemComponent.createObject(null, { "text": _rgLinkNames[i] + " " + _rgLinkStatus[i] })
            _rgMenuItems.push(menuItem)
            linkSelectionMenu.insertItem(i, menuItem)
        }

        showIndicator = _rgLinkNames.length > 1
    }

    Component.onCompleted:  updateLinkSelectionMenu()
    on_RgLinkNamesChanged:  updateLinkSelectionMenu()
    on_RgLinkStatusChanged: updateLinkSelectionMenu()

    QGCLabel {
        id:                     primaryLinkSelector
        anchors.verticalCenter: parent.verticalCenter
        text:                   _activeVehicle ? _activeVehicle.vehicleLinkManager.primaryLinkName : ""
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText

        MouseArea {
            anchors.fill:   parent
            onClicked:      linkSelectionMenu.popup()
        }
    }

    QGCMenu { id: linkSelectionMenu }

    Component {
        id: linkSelectionMenuItemComponent
        QGCMenuItem {
            onTriggered: _activeVehicle.vehicleLinkManager.primaryLinkName = text
        }
    }
}
