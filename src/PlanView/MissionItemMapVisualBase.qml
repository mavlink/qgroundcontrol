import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Base component for mission item map visuals providing common drag/indicator infrastructure.
/// Subclasses must provide an indicatorComponent.
Item {
    id: control

    property var map ///< Map control to place item in
    property var vehicle ///< Vehicle associated with this item
    property bool interactive: true

    /// Subclasses must set this to their indicator Component
    property Component indicatorComponent

    property var _missionItem: object
    property bool _itemVisualShowing: false
    property bool _dragAreaShowing: false

    signal clicked(int sequenceNumber)

    function hideItemVisuals() {
        _hideItemVisuals()
    }

    function showItemVisuals() {
        _showItemVisuals()
    }

    function hideDragArea() {
        if (_dragAreaShowing) {
            dragAreaLoader.active = false
            _dragAreaShowing = false
        }
    }

    function showDragArea() {
        if (!_dragAreaShowing) {
            dragAreaLoader.active = true
            _dragAreaShowing = true
        }
    }

    function updateDragArea() {
        if (_missionItem.isCurrentItem && map.planView && _missionItem.specifiesCoordinate) {
            showDragArea()
        } else {
            hideDragArea()
        }
    }

    function _hideItemVisuals() {
        if (_itemVisualShowing) {
            itemVisualLoader.active = false
            _itemVisualShowing = false
        }
    }

    function _showItemVisuals() {
        if (!_itemVisualShowing) {
            itemVisualLoader.active = true
            _itemVisualShowing = true
        }
    }

    Component.onCompleted: {
        showItemVisuals()
        updateDragArea()
    }

    Connections {
        target: _missionItem

        function onIsCurrentItemChanged() { updateDragArea() }
        function onSpecifiesCoordinateChanged() { updateDragArea() }
    }

    Loader {
        id: dragAreaLoader

        asynchronous: true
        active: false

        sourceComponent: dragAreaComponent

        onLoaded: {
            if (item) {
                item.parent = map
            }
        }
    }

    Loader {
        id: itemVisualLoader

        asynchronous: true
        active: false

        sourceComponent: control.indicatorComponent

        onLoaded: {
            if (item) {
                item.parent = map
                map.addMapItem(item)
            }
        }
    }

    // Control which is used to drag items
    Component {
        id: dragAreaComponent

        MissionItemIndicatorDrag {
            mapControl: control.map
            itemIndicator: itemVisualLoader.item
            itemCoordinate: _missionItem.coordinate
            visible: control.interactive
            onItemCoordinateChanged: _missionItem.coordinate = itemCoordinate
        }
    }
}
