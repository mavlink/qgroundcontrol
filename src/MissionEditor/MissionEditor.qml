/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

import QtQuick                  2.4
import QtQuick.Controls         1.3
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Mission Editor
Item {
    id: root

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property var _activeVehicle: multiVehicleManager.activeVehicle

    readonly property real _defaultLatitude:        37.803784
    readonly property real _defaultLongitude:       -122.462276

    FlightMap {
        id:                 editorMap
        anchors.fill:       parent
        mapName:            "MissionEditor"
        latitude:           parent._defaultLatitude
        longitude:          parent._defaultLongitude

        MouseArea {
            anchors.fill: parent

            onClicked: controller.addMissionItem(editorMap.mapItem.toCoordinate(Qt.point(mouse.x, mouse.y)))
        }
    }

    Column {
        id:                 controlWidgets
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.right:      parent.left
        anchors.bottom:     parent.top
        spacing:            ScreenTools.defaultFontPixelWidth / 2

        QGCButton {
            id:         addMode
            text:       "+"
            checkable:  true
        }
    }

    // Mission item list
    ListView {
        id:                 missionItemSummaryList
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.bottom:     parent.bottom
        width:              parent.width
        height:             ScreenTools.defaultFontPixelHeight * 7
        spacing:            ScreenTools.defaultFontPixelWidth / 2
        opacity:            0.75
        orientation:        ListView.Horizontal
        model:              controller.missionItems

        property real _maxItemHeight: 0

        delegate:
            MissionItemSummary {
                opacity:        0.75
                missionItem:    object
Component.onCompleted: console.log("add", object.id)
            }
    }

}
