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

/// Fixed Wing Landing Pattern map visuals
Item {
    id: _root

    property var map        ///< Map control to place item in
    property bool interactive: true

    signal clicked(int sequenceNumber)

    readonly property real _landingWidthMeters:     15
    readonly property real _landingLengthMeters:    100

    property var    _missionItem:                   object
    property var    _mouseArea
    property var    _dragAreas:                     [ ]
    property var    _flightPath
    property var    _loiterPointObject
    property var    _landingPointObject
    property real   _transitionAltitudeMeters
    property real   _midSlopeAltitudeMeters
    property real   _landingAltitudeMeters:         _missionItem.landingAltitude.rawValue
    property real   _finalApproachAltitudeMeters:   _missionItem.finalApproachAltitude.rawValue
    property bool   _useLoiterToAlt:                _missionItem.useLoiterToAlt.rawValue
    property real   _landingAreaBearing:            _missionItem.landingCoordinate.azimuthTo(_useLoiterToAlt ? _missionItem.loiterTangentCoordinate : _missionItem.finalApproachCoordinate)

    function _calcGlideSlopeHeights() {
        var adjacent
        if (_useLoiterToAlt) {
            adjacent = _missionItem.landingCoordinate.distanceTo(_missionItem.loiterTangentCoordinate)
        } else {
            adjacent = _missionItem.landingCoordinate.distanceTo(_missionItem.finalApproachCoordinate)
        }
        var opposite = _finalApproachAltitudeMeters - _landingAltitudeMeters
        var angleRadians = Math.atan(opposite / adjacent)
        var transitionDistance = _landingLengthMeters / 2
        var glideSlopeDistance = adjacent - transitionDistance

        _transitionAltitudeMeters = Math.tan(angleRadians) * (transitionDistance)
        _midSlopeAltitudeMeters = Math.tan(angleRadians) * (transitionDistance + (glideSlopeDistance / 2))
    }

    function hideItemVisuals() {
        objMgr.destroyObjects()
    }

    function showItemVisuals() {
        if (objMgr.rgDynamicObjects.length === 0) {
            _loiterPointObject = objMgr.createObject(finalApproachComponent, map, true /* parentObjectIsMap */)
            _landingPointObject = objMgr.createObject(landingPointComponent, map, true /* parentObjectIsMap */)

            var rgComponents = [ flightPathComponent, loiterRadiusComponent, landingAreaComponent, landingAreaLabelComponent,
                                glideSlopeComponent, glideSlopeLabelComponent, transitionHeightComponent, midGlideSlopeHeightComponent,
                                approachHeightComponent ]
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
            _dragAreas.push(finalApproachDragAreaComponent.createObject(map))
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

    on_LandingAltitudeMetersChanged:        _calcGlideSlopeHeights()
    on_FinalApproachAltitudeMetersChanged:  _calcGlideSlopeHeights()
    on_UseLoiterToAltChanged:               { _calcGlideSlopeHeights(); _setFlightPath() }

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

        onLandingCoordinateChanged: {
            _calcGlideSlopeHeights()
            _setFlightPath()
        }

        onLoiterTangentCoordinateChanged: {
            _calcGlideSlopeHeights()
            _setFlightPath()
        }

        onFinalApproachCoordinateChanged: {
            _calcGlideSlopeHeights()
            _setFlightPath()
        }
    }

    // Mouse area to capture landing point coordindate
    Component {
        id:  mouseAreaComponent

        MouseArea {
            anchors.fill:   map
            z:              QGroundControl.zOrderMapItems + 1   // Over item indicators
            visible:        _root.interactive

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
        id: finalApproachDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl:     _root.map
            itemIndicator:  _loiterPointObject
            itemCoordinate: _missionItem.finalApproachCoordinate
            visible:        _root.interactive

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
            visible:        _root.interactive

            onItemCoordinateChanged: _missionItem.moveLandingPosition(itemCoordinate)
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
        id: finalApproachComponent

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

    Component {
        id: landingAreaLabelComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.contentWidth / 2
            anchorPoint.y:  sourceItem.contentHeight / 2
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.landingCoordinate
            visible:        _missionItem.isCurrentItem

            sourceItem: QGCLabel {
                id:     landingAreaLabel
                text:   qsTr("Landing Area")
                color:  "white"

                property real _rawBearing:      _landingAreaBearing
                property real _adjustedBearing

                on_RawBearingChanged: {
                    _adjustedBearing = _rawBearing
                    if (_adjustedBearing > 180) {
                        _adjustedBearing -= 180
                    }
                    _adjustedBearing -= 90
                    if (_adjustedBearing < 0) {
                        _adjustedBearing += 360
                    }
                }

                transform: Rotation {
                    origin.x:   landingAreaLabel.width / 2
                    origin.y:   landingAreaLabel.height / 2
                    angle:      landingAreaLabel._adjustedBearing
                }
            }
        }
    }

    Component {
        id: glideSlopeLabelComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem._rawBearing > 180 ? sourceItem.contentWidth : 0
            anchorPoint.y:  sourceItem.contentHeight / 2
            z:              QGroundControl.zOrderMapItems
            visible:        _missionItem.isCurrentItem


            sourceItem: QGCLabel {
                id:     glideSlopeLabel
                text:   qsTr("Glide Slope")
                color:  "white"

                property real _rawBearing:      _landingAreaBearing
                property real _adjustedBearing

                on_RawBearingChanged: {
                    _adjustedBearing = _rawBearing
                    if (_adjustedBearing > 180) {
                        _adjustedBearing -= 180
                    }
                    _adjustedBearing -= 90
                    if (_adjustedBearing < 0) {
                        _adjustedBearing += 360
                    }
                }

                transform: Rotation {
                    origin.x:   sourceItem._rawBearing > 180 ? sourceItem.contentWidth : 0
                    origin.y:   glideSlopeLabel.contentHeight / 2
                    angle:      glideSlopeLabel._adjustedBearing
                }
            }

            function recalc() {
                coordinate = _missionItem.landingCoordinate.atDistanceAndAzimuth(_landingLengthMeters / 2 + 2, _landingAreaBearing)
            }

            Component.onCompleted: recalc()

            Connections {
                target:                             _missionItem
                onLandingCoordinateChanged:         recalc()
                onLoiterTangentCoordinateChanged:   recalc()
                onFinalApproachCoordinateChanged:   recalc()
            }
        }
    }

    Component {
        id: landingAreaComponent

        MapPolygon {
            z:              QGroundControl.zOrderMapItems
            border.width:   1
            border.color:   "black"
            color:          "green"
            opacity:        0.5

            readonly property real angleRadians:    Math.atan((_landingWidthMeters / 2) / (_landingLengthMeters / 2))
            readonly property real angleDegrees:    (angleRadians * (180 / Math.PI))
            readonly property real hypotenuse:      (_landingWidthMeters / 2) / Math.sin(angleRadians)

            function recalc() {
                path = [ ]
                addCoordinate(_missionItem.landingCoordinate.atDistanceAndAzimuth(hypotenuse, _landingAreaBearing - angleDegrees))
                addCoordinate(_missionItem.landingCoordinate.atDistanceAndAzimuth(hypotenuse, _landingAreaBearing + angleDegrees))
                addCoordinate(_missionItem.landingCoordinate.atDistanceAndAzimuth(hypotenuse, _landingAreaBearing + (180 - angleDegrees)))
                addCoordinate(_missionItem.landingCoordinate.atDistanceAndAzimuth(hypotenuse, _landingAreaBearing - (180 - angleDegrees)))
            }

            Component.onCompleted: recalc()

            Connections {
                target:                             _missionItem
                onLandingCoordinateChanged:         recalc()
                onLoiterTangentCoordinateChanged:   recalc()
                onFinalApproachCoordinateChanged:   recalc()
            }
        }
    }

    Component {
        id: glideSlopeComponent

        MapPolygon {
            z:              QGroundControl.zOrderMapItems
            border.width:   1
            border.color:   "black"
            color:          _missionItem.terrainCollision ? "red" : "orange"
            opacity:        0.5

            readonly property real angleRadians:    Math.atan((_landingWidthMeters / 2) / (_landingLengthMeters / 2))
            readonly property real angleDegrees:    (angleRadians * (180 / Math.PI))
            readonly property real hypotenuse:      (_landingWidthMeters / 2) / Math.sin(angleRadians)

            function recalc() {
                path = [ ]
                addCoordinate(_missionItem.landingCoordinate.atDistanceAndAzimuth(hypotenuse, _landingAreaBearing - angleDegrees))
                addCoordinate(_missionItem.landingCoordinate.atDistanceAndAzimuth(hypotenuse, _landingAreaBearing + angleDegrees))
                addCoordinate(_useLoiterToAlt ? _missionItem.loiterTangentCoordinate : _missionItem.finalApproachCoordinate)
            }

            Component.onCompleted: recalc()

            Connections {
                target:                             _missionItem
                onLandingCoordinateChanged:         recalc()
                onLoiterTangentCoordinateChanged:   recalc()
                onFinalApproachCoordinateChanged:   recalc()
            }

            Connections {
                target:             _missionItem.useLoiterToAlt
                onRawValueChanged:  recalc()
            }
        }
    }

    Component {
        id: transitionHeightComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.width / 2
            anchorPoint.y:  0
            z:              QGroundControl.zOrderMapItems
            visible:        _missionItem.isCurrentItem

            sourceItem: HeightIndicator {
                map:        _root.map
                heightText: Math.floor(QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_transitionAltitudeMeters)) +
                            QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString + "<sup>*</sup>"
            }

            function recalc() {
                var centeredCoordinate = _missionItem.landingCoordinate.atDistanceAndAzimuth(_landingLengthMeters / 2, _landingAreaBearing)
                var angleIncrement = _landingAreaBearing > 180 ? -90 : 90
                coordinate = centeredCoordinate.atDistanceAndAzimuth(_landingWidthMeters, _landingAreaBearing + angleIncrement)
            }

            Component.onCompleted: recalc()

            Connections {
                target:                             _missionItem
                onLandingCoordinateChanged:         recalc()
                onLoiterTangentCoordinateChanged:   recalc()
                onFinalApproachCoordinateChanged:   recalc()
            }
        }
    }

    Component {
        id: midGlideSlopeHeightComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.width / 2
            anchorPoint.y:  0
            z:              QGroundControl.zOrderMapItems
            visible:        _missionItem.isCurrentItem

            sourceItem: HeightIndicator {
                map:        _root.map
                heightText: Math.floor(QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_midSlopeAltitudeMeters)) +
                            QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString + "<sup>*</sup>"
            }

            function recalc() {
                var transitionCoordinate = _missionItem.landingCoordinate.atDistanceAndAzimuth(_landingLengthMeters / 2, _landingAreaBearing)
                var halfDistance = transitionCoordinate.distanceTo(_useLoiterToAlt ? _missionItem.loiterTangentCoordinate : _missionItem.finalApproachCoordinate) / 2
                var centeredCoordinate = transitionCoordinate.atDistanceAndAzimuth(halfDistance, _landingAreaBearing)
                var angleIncrement = _landingAreaBearing > 180 ? -90 : 90
                coordinate = centeredCoordinate.atDistanceAndAzimuth(_landingWidthMeters / 2, _landingAreaBearing + angleIncrement)
            }

            Component.onCompleted: recalc()

            Connections {
                target:                             _missionItem
                onLandingCoordinateChanged:         recalc()
                onLoiterTangentCoordinateChanged:   recalc()
                onFinalApproachCoordinateChanged:   recalc()
            }

            Connections {
                target:             _missionItem.useLoiterToAlt
                onRawValueChanged:  recalc()
            }
        }
    }

    Component {
        id: approachHeightComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.width / 2
            anchorPoint.y:  0
            z:              QGroundControl.zOrderMapItems
            visible:        _missionItem.isCurrentItem
            coordinate:     _useLoiterToAlt ? _missionItem.loiterTangentCoordinate : _missionItem.finalApproachCoordinate

            sourceItem: HeightIndicator {
                map:        _root.map
                heightText: _missionItem.finalApproachAltitude.value.toFixed(1) + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
            }
        }
    }
}
