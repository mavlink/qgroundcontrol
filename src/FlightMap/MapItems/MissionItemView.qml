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

import QGroundControl               1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Controls      1.0

/// The MissionItemView control is used to add Mission Item Indicators to a FlightMap.
MapItemView {
    id: _root

    delegate: MissionItemIndicator {
        id:             itemIndicator
        coordinate:     object.coordinate
        visible:        object.specifiesCoordinate && (index != 0 || object.showHomePosition)
        z:              QGroundControl.zOrderMapItems
        missionItem:    object
        sequenceNumber: object.sequenceNumber
        onClicked: {
            parent._retaskSequence = object.sequenceNumber
            parent.flightWidgets.guidedModeBar.confirmAction(parent.flightWidgets.guidedModeBar.confirmRetask)
        }

        // These are the non-coordinate child mission items attached to this item
        Row {
            anchors.top:    parent.top
            anchors.left:   parent.right

            Repeater {
                model: object.childItems

                delegate: MissionItemIndexLabel {
                    label:          object.sequenceNumber
                    isCurrentItem:  object.isCurrentItem
                    z:              2
                }
            }
        }
    }
}
