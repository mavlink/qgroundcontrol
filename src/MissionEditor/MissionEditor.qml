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

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtQuick.Dialogs  1.2
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Mission Editor

Item {
    // For some reason we can't have FlightMap as the top level control. If you do that it doesn't draw.
    // So we have an Item at the top to work around that.

    readonly property real  _defaultLatitude:   37.803784
    readonly property real  _defaultLongitude:  -122.462276
    readonly property int   _decimalPlaces:     7

    FlightMap {
        id:             editorMap
        anchors.fill:   parent
        mapName:        "MissionEditor"
        latitude:       _defaultLatitude
        longitude:      _defaultLongitude

        QGCLabel { text: "WIP: Non functional"; font.pixelSize: ScreenTools.largeFontPixelSize }


        MouseArea {
            anchors.fill: parent

            onClicked: {
                var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                coordinate.altitude = 0
                controller.addMissionItem(coordinate)
            }
        }

        // Add the mission items to the map
        MapItemView {
            model: controller.missionItems
            
            delegate:
                MissionItemIndicator {
                    label:          object.sequenceNumber
                    isCurrentItem:  object.isCurrentItem
                    coordinate:     object.coordinate

                    Component.onCompleted: console.log("Indicator", object.coordinate)
                }
        }

        // Mission item list
        ListView {
            id:                 missionItemSummaryList
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.top:        parent.top
            anchors.bottom:     editorMap.mapWidgets.top
            anchors.right:      parent.right
            width:              ScreenTools.defaultFontPixelWidth * 30
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            orientation:        ListView.Vertical
            model:              controller.missionItems

            property real _maxItemHeight: 0

            delegate:
                MissionItemEditor {
                    missionItem:    object
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
    }
}
