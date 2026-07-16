import QtQuick
import QtLocation

import QGroundControl
import QGroundControl.Controls
import QGroundControl.PlanView

/// Marker for displaying a mission item on the map
MapQuickItem {
    id: _item

    property var missionItem
    property int sequenceNumber

    readonly property bool _isCurrentItem: missionItem ? missionItem.isCurrentItem || missionItem.hasCurrentChildItem : false

    signal clicked

    anchorPoint.x:  sourceItem.anchorPointX
    anchorPoint.y:  sourceItem.anchorPointY
    z:              QGroundControl.zOrderMapItems + (_isCurrentItem ? 0.5 : 0) // Show current item above other indicators, but below controls

    sourceItem:
        MissionItemIndexLabel {
            id:                 _label
            checked:            _item._isCurrentItem
            label:              missionItem.abbreviation
            index:              missionItem.abbreviation.charAt(0) > 'A' && missionItem.abbreviation.charAt(0) < 'z' ? -1 : missionItem.sequenceNumber
            gimbalYaw:          missionItem.missionGimbalYaw
            vehicleYaw:         missionItem.missionVehicleYaw
            showGimbalYaw:      !isNaN(missionItem.missionGimbalYaw)
            highlightSelected:  true
            onClicked:          _item.clicked()
            opacity:            _item.opacity
        }
}
