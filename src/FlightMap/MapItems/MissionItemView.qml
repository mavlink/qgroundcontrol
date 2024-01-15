/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.FlightMap
import QGroundControl.Controls

/// The MissionItemView control is used to add Mission Item Indicators to a FlightMap.
MapItemView {
    id: _root

    delegate: MissionItemIndicator {
        id:             itemIndicator
        coordinate:     object.coordinate
        visible:        object.specifiesCoordinate
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
                model:      object.childItems
                delegate: MissionItemIndexLabel {
                    label:  object.abbreviation
                    checked: object.isCurrentItem
                    z:      2
                }
            }
        }
    }
}
