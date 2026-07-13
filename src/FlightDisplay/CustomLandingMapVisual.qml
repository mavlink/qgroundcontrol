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
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.ScreenTools

/// Map editing and preview visuals for the in-flight Custom Landing mode.
/// Coordinates are changed locally through CustomLandingController. No command
/// is sent to the vehicle until CustomLandingPanel calls controller.execute().
Item {
    id: _root

    property var map
    property var controller
    property bool active: false

    readonly property bool loiterCoordinateValid: _coordinateValid(controller ? controller.loiterCoordinate : undefined)
    readonly property bool landingCoordinateValid: _coordinateValid(controller ? controller.landingCoordinate : undefined)
    readonly property bool draftComplete: loiterCoordinateValid && landingCoordinateValid
    readonly property bool interactive: active && controller && controller.modeActive && controller.capabilitySupported
                                        && !controller.busy && !controller.planCommitted
    readonly property real loiterRadiusMeters: controller ? Math.max(0, Number(controller.loiterRadius)) : 0
    readonly property real minimumApproachMarginMeters: 30
    readonly property real centerToLandingDistance: draftComplete
                                                        ? controller.loiterCoordinate.distanceTo(controller.landingCoordinate)
                                                        : 0
    readonly property bool geometryValid: draftComplete && loiterRadiusMeters > 0
                                             && centerToLandingDistance > loiterRadiusMeters + minimumApproachMarginMeters
    readonly property string geometryError: draftComplete && !geometryValid
                                                ? qsTr("Landing point must be at least 30 metres beyond the loiter radius.")
                                                : ""
    readonly property var tangentCoordinate: _calculateTangentCoordinate()
    readonly property var approachPath: geometryValid
                                            ? [tangentCoordinate, controller.landingCoordinate]
                                            : []

    property var _mapClickArea
    property var _loiterMarker
    property var _landingMarker
    property var _dragAreas: []

    QGCDynamicObjectManager {
        id: visualObjectManager
    }

    function _coordinateValid(coordinate) {
        return coordinate !== undefined && coordinate !== null && coordinate.isValid
    }

    function _normalizedBearing(bearing) {
        var normalized = bearing % 360
        return normalized < 0 ? normalized + 360 : normalized
    }

    // For an external point P and circle center C, the tangent radius differs
    // from bearing(C,P) by acos(radius / distance(C,P)). The sign selects the
    // tangent whose direction of travel points toward P.
    function _calculateTangentCoordinate() {
        if (!geometryValid) {
            return QtPositioning.coordinate()
        }

        var center = controller.loiterCoordinate
        var landing = controller.landingCoordinate
        var offsetDegrees = Math.acos(loiterRadiusMeters / centerToLandingDistance) * 180 / Math.PI
        var centerToLandingBearing = center.azimuthTo(landing)
        var tangentBearing = controller.clockwise
                ? centerToLandingBearing - offsetDegrees
                : centerToLandingBearing + offsetDegrees
        var tangent = center.atDistanceAndAzimuth(loiterRadiusMeters, _normalizedBearing(tangentBearing))
        tangent.altitude = Number(controller.loiterAltitude)
        return tangent
    }

    function _circleCoordinate(bearing) {
        if (!loiterCoordinateValid || loiterRadiusMeters <= 0) {
            return QtPositioning.coordinate()
        }
        var coordinate = controller.loiterCoordinate.atDistanceAndAzimuth(loiterRadiusMeters,
                                                                           _normalizedBearing(bearing))
        coordinate.altitude = Number(controller.loiterAltitude)
        return coordinate
    }

    function _roundedMapCoordinate(point, altitude) {
        var coordinate = map.toCoordinate(point, false /* clipToViewport */)
        coordinate.latitude = Number(coordinate.latitude.toFixed(7))
        coordinate.longitude = Number(coordinate.longitude.toFixed(7))
        coordinate.altitude = Number(altitude)
        return coordinate
    }

    function _showVisuals() {
        if (!active || !map || visualObjectManager.rgDynamicObjects.length > 0) {
            return
        }

        _loiterMarker = visualObjectManager.createObject(loiterMarkerComponent, map, true /* parentObjectIsMap */)
        _landingMarker = visualObjectManager.createObject(landingMarkerComponent, map, true /* parentObjectIsMap */)

        visualObjectManager.createObjects([
            loiterCircleComponent,
            approachLineComponent,
            tangentMarkerComponent,
            directionArrowZeroComponent,
            directionArrowOneTwentyComponent,
            directionArrowTwoFortyComponent
        ], map, true /* parentObjectIsMap */)

        _showDragAreas()
    }

    function _hideVisuals() {
        _hideMapClickArea()
        _hideDragAreas()
        visualObjectManager.destroyObjects()
        _loiterMarker = undefined
        _landingMarker = undefined
    }

    function _showDragAreas() {
        if (!active || !map || _dragAreas.length > 0 || !_loiterMarker || !_landingMarker) {
            return
        }
        _dragAreas.push(loiterDragAreaComponent.createObject(map))
        _dragAreas.push(landingDragAreaComponent.createObject(map))
    }

    function _hideDragAreas() {
        for (var i = 0; i < _dragAreas.length; i++) {
            _dragAreas[i].destroy()
        }
        _dragAreas = []
    }

    function _showMapClickArea() {
        if (!_mapClickArea && active && interactive && (!loiterCoordinateValid || !landingCoordinateValid)) {
            _mapClickArea = mapClickAreaComponent.createObject(map)
        }
    }

    function _hideMapClickArea() {
        if (_mapClickArea) {
            _mapClickArea.destroy()
            _mapClickArea = undefined
        }
    }

    function _updateInteractionObjects() {
        if (!active) {
            _hideVisuals()
            return
        }

        _showVisuals()
        if (interactive && (!loiterCoordinateValid || !landingCoordinateValid)) {
            _showMapClickArea()
        } else {
            _hideMapClickArea()
        }
    }

    onActiveChanged: Qt.callLater(_updateInteractionObjects)
    onMapChanged: Qt.callLater(function() {
        _hideVisuals()
        _updateInteractionObjects()
    })
    onInteractiveChanged: Qt.callLater(_updateInteractionObjects)
    onLoiterCoordinateValidChanged: Qt.callLater(_updateInteractionObjects)
    onLandingCoordinateValidChanged: Qt.callLater(_updateInteractionObjects)

    Component.onCompleted: _updateInteractionObjects()
    Component.onDestruction: _hideVisuals()

    Connections {
        target: controller
        ignoreUnknownSignals: true

        function onLoiterCoordinateChanged() {
            Qt.callLater(_root._updateInteractionObjects)
        }

        function onLandingCoordinateChanged() {
            Qt.callLater(_root._updateInteractionObjects)
        }

        function onBusyChanged() {
            Qt.callLater(_root._updateInteractionObjects)
        }

        function onPlanCommittedChanged() {
            Qt.callLater(_root._updateInteractionObjects)
        }
    }

    Component {
        id: mapClickAreaComponent

        MouseArea {
            anchors.fill: map
            z: QGroundControl.zOrderMapItems + 20
            acceptedButtons: Qt.LeftButton
            visible: _root.active && _root.interactive
                     && (!_root.loiterCoordinateValid || !_root.landingCoordinateValid)

            onClicked: (mouse) => {
                if (!_root.loiterCoordinateValid) {
                    _root.controller.loiterCoordinate = _root._roundedMapCoordinate(
                                Qt.point(mouse.x, mouse.y), _root.controller.loiterAltitude)
                } else if (!_root.landingCoordinateValid) {
                    _root.controller.landingCoordinate = _root._roundedMapCoordinate(
                                Qt.point(mouse.x, mouse.y), _root.controller.landingAltitude)
                }
                Qt.callLater(_root._updateInteractionObjects)
            }
        }
    }

    Component {
        id: loiterDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl: map
            itemIndicator: _root._loiterMarker
            itemCoordinate: _root.controller ? _root.controller.loiterCoordinate : QtPositioning.coordinate()
            visible: _root.interactive && _root.loiterCoordinateValid

            onItemCoordinateChanged: {
                if (Drag.active && _root.interactive) {
                    var coordinate = itemCoordinate
                    coordinate.altitude = Number(_root.controller.loiterAltitude)
                    _root.controller.loiterCoordinate = coordinate
                }
            }
        }
    }

    Component {
        id: landingDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl: map
            itemIndicator: _root._landingMarker
            itemCoordinate: _root.controller ? _root.controller.landingCoordinate : QtPositioning.coordinate()
            visible: _root.interactive && _root.landingCoordinateValid

            onItemCoordinateChanged: {
                if (Drag.active && _root.interactive) {
                    var coordinate = itemCoordinate
                    coordinate.altitude = Number(_root.controller.landingAltitude)
                    _root.controller.landingCoordinate = coordinate
                }
            }
        }
    }

    Component {
        id: loiterCircleComponent

        MapCircle {
            z: QGroundControl.zOrderMapItems - 2
            center: _root.controller ? _root.controller.loiterCoordinate : QtPositioning.coordinate()
            radius: _root.loiterRadiusMeters
            border.width: 3
            border.color: "#f4b942"
            color: Qt.rgba(0.96, 0.73, 0.26, 0.12)
            visible: _root.active && _root.loiterCoordinateValid && _root.loiterRadiusMeters > 0
        }
    }

    Component {
        id: approachLineComponent

        MapPolyline {
            z: QGroundControl.zOrderMapItems - 1
            line.color: "#ff7b32"
            line.width: 4
            path: _root.approachPath
            visible: _root.active && _root.geometryValid
        }
    }

    Component {
        id: tangentMarkerComponent

        MapQuickItem {
            z: QGroundControl.zOrderMapItems
            coordinate: _root.tangentCoordinate
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height / 2
            visible: _root.active && _root.geometryValid

            sourceItem: Rectangle {
                width: Math.max(12, ScreenTools.defaultFontPixelWidth * 1.1)
                height: width
                radius: width / 2
                color: "#ff7b32"
                border.width: 2
                border.color: "white"

                QGCLabel {
                    anchors.left: parent.right
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.4
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Tangent exit")
                    color: "white"
                    style: Text.Outline
                    styleColor: "black"
                }
            }
        }
    }

    component DirectionArrow: MapQuickItem {
        property real radialBearing: 0

        z: QGroundControl.zOrderMapItems
        coordinate: _root._circleCoordinate(radialBearing)
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        visible: _root.active && _root.loiterCoordinateValid && _root.loiterRadiusMeters > 0

        sourceItem: QGCLabel {
            text: "\u27a4"
            color: "#f4b942"
            font.bold: true
            font.pointSize: ScreenTools.defaultFontPointSize * 1.5
            style: Text.Outline
            styleColor: "black"
            // The glyph points east at zero rotation. A clockwise tangent at
            // radial bearing b points b+90 degrees, hence rotation=b.
            rotation: _root.controller && _root.controller.clockwise
                          ? radialBearing
                          : radialBearing + 180
        }
    }

    Component {
        id: directionArrowZeroComponent
        DirectionArrow { radialBearing: 0 }
    }

    Component {
        id: directionArrowOneTwentyComponent
        DirectionArrow { radialBearing: 120 }
    }

    Component {
        id: directionArrowTwoFortyComponent
        DirectionArrow { radialBearing: 240 }
    }

    component LandingMarker: Item {
        id: markerRoot

        property color markerColor: "#f4b942"
        property string title
        property string altitudeText
        property real anchorPointX: width / 2
        property real anchorPointY: height - markerDot.height / 2

        width: Math.max(markerLabel.implicitWidth + ScreenTools.defaultFontPixelWidth,
                        ScreenTools.minTouchPixels)
        height: markerLabel.implicitHeight + markerDot.height + ScreenTools.defaultFontPixelHeight * 0.2

        Rectangle {
            id: markerLabel
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: markerText.implicitWidth + ScreenTools.defaultFontPixelWidth
            height: markerText.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.35
            radius: ScreenTools.defaultFontPixelWidth * 0.45
            color: Qt.rgba(0.06, 0.08, 0.11, 0.9)
            border.width: 2
            border.color: markerRoot.markerColor

            QGCLabel {
                id: markerText
                anchors.centerIn: parent
                text: markerRoot.title + "\n" + markerRoot.altitudeText
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
            }
        }

        Rectangle {
            id: markerDot
            anchors.top: markerLabel.bottom
            anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.2
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(16, ScreenTools.defaultFontPixelWidth * 1.35)
            height: width
            radius: width / 2
            color: markerRoot.markerColor
            border.width: 2
            border.color: "white"
        }
    }

    Component {
        id: loiterMarkerComponent

        MapQuickItem {
            id: loiterMapItem

            z: QGroundControl.zOrderMapItems + 1
            coordinate: _root.controller ? _root.controller.loiterCoordinate : QtPositioning.coordinate()
            anchorPoint.x: loiterMarkerSource.anchorPointX
            anchorPoint.y: loiterMarkerSource.anchorPointY
            visible: _root.active && _root.loiterCoordinateValid

            sourceItem: LandingMarker {
                id: loiterMarkerSource
                markerColor: "#f4b942"
                title: qsTr("Loiter descent")
                altitudeText: qsTr("%1 m").arg(Number(_root.controller.loiterAltitude).toFixed(1))
            }
        }
    }

    Component {
        id: landingMarkerComponent

        MapQuickItem {
            id: landingMapItem

            z: QGroundControl.zOrderMapItems + 1
            coordinate: _root.controller ? _root.controller.landingCoordinate : QtPositioning.coordinate()
            anchorPoint.x: landingMarkerSource.anchorPointX
            anchorPoint.y: landingMarkerSource.anchorPointY
            visible: _root.active && _root.landingCoordinateValid

            sourceItem: LandingMarker {
                id: landingMarkerSource
                markerColor: "#4bc0ff"
                title: qsTr("Vertical land")
                altitudeText: qsTr("%1 m").arg(Number(_root.controller.landingAltitude).toFixed(1))
            }
        }
    }
}
