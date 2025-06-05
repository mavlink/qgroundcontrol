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
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Simple Mission Item visuals
Item {
    id: _root

    property var map        ///< Map control to place item in
    property var vehicle    ///< Vehicle associated with this item
    property bool interactive: true

    property var    _missionItem:       object
    property bool   _itemVisualShowing: false
    property bool   _dragAreaShowing:   false

    signal clicked(int sequenceNumber)

    function hideItemVisuals() {
        if (_itemVisualShowing) {
            itemVisualLoader.active = false
            loiterVisualLoader.active = false
            _itemVisualShowing = false
        }
    }

    function showItemVisuals() {
        if (!_itemVisualShowing) {
            itemVisualLoader.active = true
            loiterVisualLoader.active = true
            _itemVisualShowing = true
        }
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

    Component.onCompleted: {
        showItemVisuals()
        updateDragArea()
    }

    Connections {
        target: _missionItem

        function onIsCurrentItemChanged() {         updateDragArea() }
        function onSpecifiesCoordinateChanged() {   updateDragArea() }
    }

    Connections {
        target: _missionItem.isSimpleItem ? _missionItem : null

        onLoiterRadiusChanged: {
            if (loiterVisualLoader.item) {
                loiterVisualLoader.item.handleLoiterRadiusChange()
            }
        }

        onCoordinateChanged: {
            if (loiterVisualLoader.item) {
                loiterVisualLoader.item.handleCoordinateChange()
            }
        }
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

        sourceComponent: indicatorComponent

        onLoaded: {
            if (item) {
                item.parent = map
                map.addMapItem(item)
            }
        }
    }

    Loader {
        id: loiterVisualLoader

        asynchronous: true
        active: false

        sourceComponent: loiterComponent

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
            mapControl:              _root.map
            itemIndicator:           itemVisualLoader.item
            itemCoordinate:          _missionItem.coordinate
            visible:                 _root.interactive
            onItemCoordinateChanged: _missionItem.coordinate = itemCoordinate
        }
    }

    Component {
        id: indicatorComponent

        MissionItemIndicator {
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.specifiesCoordinate
            z:              QGroundControl.zOrderMapItems
            missionItem:    _missionItem
            sequenceNumber: _missionItem.sequenceNumber
            onClicked:      if(_root.interactive)  _root.clicked(_missionItem.sequenceNumber)
            opacity:        _root.opacity
        }
    }

    Component  {
        id: loiterComponent

        MapQuickItem {
            id:                               loiterMapQuickItem
            coordinate:                       _root._missionItem.coordinate
            visible:                          _root.interactive && _missionItem.isSimpleItem && _missionItem.showLoiterRadius

            property alias blockSignals:      loiterMapCircleVisuals.blockSignals
            property alias radius:            _mapCircle.radius
            property alias clockwiseRotation: _mapCircle.clockwiseRotation

            function handleLoiterRadiusChange() {
                blockSignals = true
                clockwiseRotation = _missionItem.loiterRadius>= 0
                blockSignals = false
                radius.rawValue = Math.abs(_missionItem.loiterRadius)
            }

            function handleCoordinateChange() {
                coordinate = _missionItem.coordinate
            }

            onCoordinateChanged:              _mapCircle.center = coordinate

            sourceItem: QGCMapCircleVisuals {
                id:                      loiterMapCircleVisuals
                mapControl:              _root.map
                mapCircle:               _mapCircle
                centerDragHandleVisible: false
                borderColor:             _missionItem.terrainCollision ? "red" : QGroundControl.globalPalette.mapMissionTrajectory

                property bool blockSignals: false

                function updateMissionItem() {
                    _missionItem.loiterRadius = _mapCircle.clockwiseRotation ? _mapCircle.radius.rawValue : -_mapCircle.radius.rawValue
                }

                QGCMapCircle {
                    id:                         _mapCircle
                    center:                     loiterMapQuickItem.coordinate
                    interactive:                _root.interactive && _missionItem.isCurrentItem && map.planView
                    showRotation:               true
                    onClockwiseRotationChanged: if(!blockSignals) loiterMapCircleVisuals.updateMissionItem()
                }

                Connections {
                    target:            _mapCircle.radius
                    function onRawValueChanged() {
                        if(!blockSignals) loiterMapCircleVisuals.updateMissionItem()
                    }
                }
            }

            Component.onCompleted: {
                handleLoiterRadiusChange()
                handleCoordinateChange()
            }
        }
    }
}
