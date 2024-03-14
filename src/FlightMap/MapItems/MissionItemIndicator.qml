/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtLocation

import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Vehicle

/// Marker for displaying a mission item on the map
MapQuickItem {
    id: _item

    property var missionItem
    property int sequenceNumber

    signal clicked

    anchorPoint.x:  sourceItem.anchorPointX
    anchorPoint.y:  sourceItem.anchorPointY

    sourceItem:
        MissionItemIndexLabel {
            id:                 _label
            checked:            _isCurrentItem
            label:              missionItem.abbreviation
            index:              missionItem.abbreviation.charAt(0) > 'A' && missionItem.abbreviation.charAt(0) < 'z' ? -1 : missionItem.sequenceNumber
            gimbalYaw:          missionItem.missionGimbalYaw
            vehicleYaw:         missionItem.missionVehicleYaw
            showGimbalYaw:      !isNaN(missionItem.missionGimbalYaw)
            highlightSelected:  true
            onClicked:          _item.clicked()
            opacity:            _item.opacity

            property bool _isCurrentItem:   missionItem ? missionItem.isCurrentItem || missionItem.hasCurrentChildItem : false
        }
}
