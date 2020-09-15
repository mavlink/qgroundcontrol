/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Layouts  1.11

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

Item {
    id: _root

    property var map        ///< Map control to place item in

    signal clicked(int sequenceNumber)

    readonly property real _landingWidthMeters:     15
    readonly property real _landingLengthMeters:    100

    property var    _missionItem:               object
    property var    _mouseArea
    property var    _dragAreas:                 [ ]
    property var    _flightPath
    property var    _loiterPointObject
    property var    _landingPointObject
    property bool   _useLoiterToAlt:            _missionItem.useLoiterToAlt.rawValue
    property real   _landingAreaBearing:            _missionItem.landingCoordinate.azimuthTo(_useLoiterToAlt ? _missionItem.loiterTangentCoordinate : _missionItem.finalApproachCoordinate)

    function hideItemVisuals() {
        objMgr.destroyObjects()
    }

    function showItemVisuals() {
        if (objMgr.rgDynamicObjects.length === 0) {
            _loiterPointObject = objMgr.createObject(finalApproachPointComponent, map, true /* parentObjectIsMap */)
            _landingPointObject = objMgr.createObject(landingPointComponent, map, true /* parentObjectIsMap */)

            var rgComponents = [ flightPathComponent, loiterRadiusComponent ]
            objMgr.createObjects(rgComponents, map, true /* parentObjectIsMap */)
        }
    }

    function hideMouseArea() {
        if (_mouseArea) {
            _mouseArea.destroy()
            _mouseArea = undefined
        }
    }

    function showMouseArea() {
        if (!_mouseArea) {
            _mouseArea = mouseAreaComponent.createObject(map)
            map.addMapItem(_mouseArea)
        }
    }

    function hideDragAreas() {
        for (var i=0; i<_dragAreas.length; i++) {
            _dragAreas[i].destroy()
        }
        _dragAreas = [ ]
    }

    function showDragAreas() {
        if (_dragAreas.length === 0) {
            _dragAreas.push(loiterDragAreaComponent.createObject(map))
            _dragAreas.push(landDragAreaComponent.createObject(map))
        }
    }

    function _setFlightPath() {
        if (_useLoiterToAlt) {
            _flightPath = [ _missionItem.loiterTangentCoordinate, _missionItem.landingCoordinate ]
        } else {
            _flightPath = [ _missionItem.finalApproachCoordinate, _missionItem.landingCoordinate ]
        }
    }

    QGCDynamicObjectManager {
        id: objMgr
    }

    Component.onCompleted: {
        if (_missionItem.landingCoordSet) {
            showItemVisuals()
            if (!_missionItem.flyView && _missionItem.isCurrentItem) {
                showDragAreas()
            }
            _setFlightPath()
        } else if (!_missionItem.flyView && _missionItem.isCurrentItem) {
            showMouseArea()
        }
    }

    Component.onDestruction: {
        hideDragAreas()
        hideMouseArea()
        hideItemVisuals()
    }

    on_UseLoiterToAltChanged: _setFlightPath()

    Connections {
        target: _missionItem

        onIsCurrentItemChanged: {
            if (_missionItem.flyView) {
                return
            }
            if (_missionItem.isCurrentItem) {
                if (_missionItem.landingCoordSet) {
                    showDragAreas()
                } else {
                    showMouseArea()
                }
            } else {
                hideMouseArea()
                hideDragAreas()
            }
        }

        onLandingCoordSetChanged: {
            if (_missionItem.flyView) {
                return
            }
            if (_missionItem.landingCoordSet) {
                hideMouseArea()
                showItemVisuals()
                showDragAreas()
                _setFlightPath()
            } else if (_missionItem.isCurrentItem) {
                hideDragAreas()
                showMouseArea()
            }
        }

        onLandingCoordinateChanged:         _setFlightPath()
        onLoiterTangentCoordinateChanged:   _setFlightPath()
        onFinalApproachCoordinateChanged:   _setFlightPath()
    }

    // Mouse area to capture landing point coordindate
    Component {
        id:  mouseAreaComponent

        MouseArea {
            anchors.fill:   map
            z:              QGroundControl.zOrderMapItems + 1   // Over item indicators

            readonly property int   _decimalPlaces:             8

            onClicked: {
                var coordinate = map.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                _missionItem.landingCoordinate = coordinate
                _missionItem.setLandingHeadingToTakeoffHeading()
            }
        }
    }

    // Control which is used to drag the final approach point
    Component {
        id: loiterDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl:     _root.map
            itemIndicator:  _loiterPointObject
            itemCoordinate: _missionItem.finalApproachCoordinate

            property bool _preventReentrancy: false

            onItemCoordinateChanged: {
                if (!_preventReentrancy) {
                    if (Drag.active) {
                        _preventReentrancy = true
                        var angle = _missionItem.landingCoordinate.azimuthTo(itemCoordinate)
                        var distance = _missionItem.landingCoordinate.distanceTo(_missionItem.finalApproachCoordinate)
                        _missionItem.finalApproachCoordinate = _missionItem.landingCoordinate.atDistanceAndAzimuth(distance, angle)
                        _preventReentrancy = false
                    }
                }
            }
        }
    }

    // Control which is used to drag the landing point
    Component {
        id: landDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl:     _root.map
            itemIndicator:  _landingPointObject
            itemCoordinate: _missionItem.landingCoordinate

            onItemCoordinateChanged: _missionItem.landingCoordinate = itemCoordinate
        }
    }

    // Flight path
    Component {
        id: flightPathComponent

        MapPolyline {
            z:          QGroundControl.zOrderMapItems - 1   // Under item indicators
            line.color: "#be781c"
            line.width: 2
            path:       _flightPath
        }
    }

    // Final approach point
    Component {
        id: finalApproachPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.finalApproachCoordinate

            sourceItem:
                MissionItemIndexLabel {
                index:      _missionItem.sequenceNumber
                label:      _useLoiterToAlt ? qsTr("Loiter") : qsTr("Approach")
                checked:    _missionItem.isCurrentItem

                onClicked: _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }

    // Landing point
    Component {
        id: landingPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.landingCoordinate

            sourceItem:
                MissionItemIndexLabel {
                index:      _missionItem.lastSequenceNumber
                label:      qsTr("Land")
                checked:    _missionItem.isCurrentItem

                onClicked: _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }

    Component {
        id: loiterRadiusComponent

        MapCircle {
            z:              QGroundControl.zOrderMapItems
            center:         _missionItem.finalApproachCoordinate
            radius:         _missionItem.loiterRadius.rawValue
            border.width:   2
            border.color:   "green"
            color:          "transparent"
            visible:        _useLoiterToAlt
        }
    }
}
