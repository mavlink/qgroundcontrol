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
    property MissionItemIndicatorGroup indicatorGroup
    property bool indicatorVisible: true
    property bool interactive: true

    readonly property bool _isCurrentItem: missionItem ? missionItem.isCurrentItem || missionItem.hasCurrentChildItem : false
    readonly property bool _usesAbbreviation: missionItem && missionItem.abbreviation.charAt(0) > 'A' && missionItem.abbreviation.charAt(0) < 'z'
    readonly property var _group: indicatorGroup ? indicatorGroup.groupForItem(missionItem) : null
    readonly property bool _isGrouped: _group && _group.items.length > 1
    readonly property bool _isGroupRepresentative: !_group || _group.representative === missionItem

    signal clicked

    function activate() {
        if (_isGrouped) {
            const topLeft = _label.mapToItem(globals.parent, Qt.point(0, 0))
            const bottomRight = _label.mapToItem(globals.parent, Qt.point(_label.width, _label.height))
            const clickRect = Qt.rect(topLeft.x, topLeft.y,
                                      bottomRight.x - topLeft.x, bottomRight.y - topLeft.y)
            indicatorGroup.showGroup(missionItem, clickRect)
        } else {
            clicked()
        }
    }

    anchorPoint.x:  sourceItem.anchorPointX
    anchorPoint.y:  sourceItem.anchorPointY
    autoFadeIn:     false
    z:              QGroundControl.zOrderMapItems + (_isCurrentItem ? 1 : 0)
    visible:        indicatorVisible && _isGroupRepresentative

    sourceItem:
        MissionItemIndexLabel {
            id:                 _label
            checked:            _item._isCurrentItem
            label:              _item.missionItem.abbreviation
            index:              _item._usesAbbreviation ? -1 : _item.missionItem.sequenceNumber
            indicatorSubText:   _item._isGrouped ? "…" : ""
            small:              !_item._isGrouped && !_item._isCurrentItem
            medium:             _item._isGrouped && !_item._isCurrentItem
            gimbalYaw:          missionItem.missionGimbalYaw
            vehicleYaw:         missionItem.missionVehicleYaw
            showGimbalYaw:      !_item._isGrouped && !isNaN(_item.missionItem.missionGimbalYaw)
            highlightSelected:  true
            enabled:            _item.interactive
            onClicked:          _item.activate()
            opacity:            _item.opacity
        }
}
