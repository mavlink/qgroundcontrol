import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Simple Mission Item visuals
MissionItemMapVisualBase {
    id: _root

    indicatorComponent: indicatorComponent

    function hideItemVisuals() {
        if (_itemVisualShowing) {
            _hideItemVisuals()
            loiterVisualLoader.active = false
            waypointRadiusVisualLoader.active = false
        }
    }

    function showItemVisuals() {
        if (!_itemVisualShowing) {
            _showItemVisuals()
            loiterVisualLoader.active = true
            waypointRadiusVisualLoader.active = true
        }
    }

    Connections {
        target: _missionItem.isSimpleItem ? _missionItem : null

        function onLoiterRadiusChanged(loiterRadius) {
            if (loiterVisualLoader.item) {
                loiterVisualLoader.item.handleLoiterRadiusChange()
            }
        }

        function onWaypointRadiusChanged(waypointRadius) {
            if (waypointRadiusVisualLoader.item) {
                waypointRadiusVisualLoader.item.handleWaypointRadiusChange()
            }
        }

        function onShowWaypointRadiusChanged() {
            if (waypointRadiusVisualLoader.item) {
                waypointRadiusVisualLoader.item.handleWaypointRadiusChange()
            }
        }

        function onCoordinateChanged(coordinate) {
            if (loiterVisualLoader.item) {
                loiterVisualLoader.item.handleCoordinateChange()
            }
            if (waypointRadiusVisualLoader.item) {
                waypointRadiusVisualLoader.item.handleCoordinateChange()
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

    Loader {
        id: waypointRadiusVisualLoader

        asynchronous: true
        active: false

        sourceComponent: waypointRadiusComponent

        onLoaded: {
            if (item) {
                item.parent = map
                map.addMapItem(item)
            }
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

    Component  {
        id: waypointRadiusComponent

        MapQuickItem {
            id:                               waypointRadiusMapQuickItem
            coordinate:                       _root._missionItem.coordinate
            visible:                          _root.interactive && _missionItem.isSimpleItem && _missionItem.showWaypointRadius

            property alias blockSignals:      waypointRadiusMapCircleVisuals.blockSignals
            property alias radius:            _mapCircle.radius

            function handleWaypointRadiusChange() {
                blockSignals = true
                radius.rawValue = _missionItem.waypointRadius.rawValue
                blockSignals = false
            }

            function handleCoordinateChange() {
                coordinate = _missionItem.coordinate
            }

            onCoordinateChanged:              _mapCircle.center = coordinate

            sourceItem: QGCMapCircleVisuals {
                id:                      waypointRadiusMapCircleVisuals
                mapControl:              _root.map
                mapCircle:               _mapCircle
                centerDragHandleVisible: false
                borderColor:             _missionItem.terrainCollision ? "red" : QGroundControl.globalPalette.mapMissionTrajectory

                property bool blockSignals: false

                function updateMissionItem() {
                    _missionItem.waypointRadius.rawValue = _mapCircle.radius.rawValue
                }

                QGCMapCircle {
                    id:                         _mapCircle
                    center:                     waypointRadiusMapQuickItem.coordinate
                    interactive:                _root.interactive && _missionItem.isCurrentItem && map.planView
                    showRotation:               false
                }

                Connections {
                    target:            _mapCircle.radius
                    function onRawValueChanged() {
                        if (!blockSignals) waypointRadiusMapCircleVisuals.updateMissionItem()
                    }
                }
            }

            Component.onCompleted: {
                handleWaypointRadiusChange()
                handleCoordinateChange()
            }
        }
    }
}
