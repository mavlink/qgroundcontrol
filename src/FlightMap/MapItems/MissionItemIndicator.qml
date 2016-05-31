/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick      2.4
import QtLocation   5.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Vehicle       1.0

/// Marker for displaying a mission item on the map
MapQuickItem {
    id: _item

    property var missionItem
    property int sequenceNumber

    signal clicked

    anchorPoint.x:  sourceItem.width  / 2
    anchorPoint.y:  sourceItem.height / 2

    sourceItem:
        MissionItemIndexLabel {
            id:             _label
            isCurrentItem:  _isCurrentItem
            label:          missionItem ? missionItem.abbreviation : ""
            onClicked:      _item.clicked()

            property bool _isCurrentItem:   missionItem ? missionItem.isCurrentItem : false
        }
}
