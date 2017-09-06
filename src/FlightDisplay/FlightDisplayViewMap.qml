/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0

FlightMap {
    id:                         flightMap
    anchors.fill:               parent
    mapName:                    _mapName
    allowGCSLocationCenter:     !userPanned
    allowVehicleLocationCenter: !_keepVehicleCentered
    planView:                   false

    property alias  scaleState: mapScale.state

    // The following properties must be set by the consumer
    property var    planMasterController
    property var    guidedActionsController
    property var    flightWidgets
    property var    rightPanelWidth
    property var    qgcView                             ///< QGCView control which contains this map
    property var    multiVehicleView                    ///< true: multi-vehicle view, false: single vehicle view

    property rect   centerViewport:             Qt.rect(0, 0, width, height)

    property var    _planMasterController:      planMasterController
    property var    _missionController:         _planMasterController.missionController
    property var    _geoFenceController:        _planMasterController.geoFenceController
    property var    _rallyPointController:      _planMasterController.rallyPointController
    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property var    _activeVehicleCoordinate:   _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property var    _gotoHereCoordinate:        QtPositioning.coordinate()
    property real   _toolButtonTopMargin:       parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)

    property bool   _disableVehicleTracking:    false
    property bool   _keepVehicleCentered:       _mainIsMap ? false : true

    // Track last known map position and zoom from Fly view in settings
    onZoomLevelChanged: QGroundControl.flightMapZoom = zoomLevel
    onCenterChanged:    QGroundControl.flightMapPosition = center

    // When the user pans the map we stop responding to vehicle coordinate updates until the panRecenterTimer fires
    onUserPannedChanged: {
        if (userPanned) {
            console.log("user panned")
            userPanned = false
            _disableVehicleTracking = true
            panRecenterTimer.restart()
        }
    }

    function pointInRect(point, rect) {
        return point.x > rect.x &&
                point.x < rect.x + rect.width &&
                point.y > rect.y &&
                point.y < rect.y + rect.height;
    }

    property real _animatedLatitudeStart
    property real _animatedLatitudeStop
    property real _animatedLongitudeStart
    property real _animatedLongitudeStop
    property real animatedLatitude
    property real animatedLongitude

    onAnimatedLatitudeChanged: flightMap.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)
    onAnimatedLongitudeChanged: flightMap.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)

    NumberAnimation on animatedLatitude { id: animateLat; from: _animatedLatitudeStart; to: _animatedLatitudeStop; duration: 1000 }
    NumberAnimation on animatedLongitude { id: animateLong; from: _animatedLongitudeStart; to: _animatedLongitudeStop; duration: 1000 }

    function animatedMapRecenter(fromCoord, toCoord) {
        _animatedLatitudeStart = fromCoord.latitude
        _animatedLongitudeStart = fromCoord.longitude
        _animatedLatitudeStop = toCoord.latitude
        _animatedLongitudeStop = toCoord.longitude
        animateLat.start()
        animateLong.start()
    }

    function recenterNeeded() {
        var vehiclePoint = flightMap.fromCoordinate(_activeVehicleCoordinate, false /* clipToViewport */)
        var centerViewport = Qt.rect(0, 0, width, height)
        return !pointInRect(vehiclePoint, centerViewport)
    }

    function updateMapToVehiclePosition() {
        // We let FlightMap handle first vehicle position
        if (firstVehiclePositionReceived && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            if (_keepVehicleCentered) {
                flightMap.center = _activeVehicleCoordinate
            } else {
                if (firstVehiclePositionReceived && recenterNeeded()) {
                    animatedMapRecenter(flightMap.center, _activeVehicleCoordinate)
                }
            }
        }
    }

    Timer {
        id:         panRecenterTimer
        interval:   10000
        running:    false

        onTriggered: {
            _disableVehicleTracking = false
            updateMapToVehiclePosition()
        }
    }

    Timer {
        interval:       500
        running:        true
        repeat:         true
        onTriggered:    updateMapToVehiclePosition()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    QGCMapPalette { id: mapPal; lightColors: isSatelliteMap }

    Connections {
        target: _missionController

        onNewItemsFromVehicle: {
            var visualItems = _missionController.visualItems
            if (visualItems && visualItems.count != 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
                firstVehiclePositionReceived = true
            }
        }
    }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        _flightMap
        usePlannedHomePosition:     false
        planMasterController:       _planMasterController

        property real leftToolWidth:    toolStrip.x + toolStrip.width
    }

    // Add trajectory points to the map
    MapItemView {
        model: _mainIsMap ? _activeVehicle ? _activeVehicle.trajectoryPoints : 0 : 0

        delegate: MapPolyline {
            line.width: 3
            line.color: "red"
            z:          QGroundControl.zOrderTrajectoryLines
            path: [
                object.coordinate1,
                object.coordinate2,
            ]
        }
    }

    // Add the vehicles to the map
    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles

        delegate: VehicleMapItem {
            vehicle:        object
            coordinate:     object.coordinate
            map:            flightMap
            size:           _mainIsMap ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight
            z:              QGroundControl.zOrderVehicles
        }
    }

    // Add ADSB vehicles to the map
    MapItemView {
        model: _activeVehicle ? _activeVehicle.adsbVehicles : 0

        property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

        delegate: VehicleMapItem {
            coordinate:     object.coordinate
            altitude:       object.altitude
            callsign:       object.callsign
            heading:        object.heading
            map:            flightMap
            z:              QGroundControl.zOrderVehicles
        }
    }

    // Add the items associated with each vehicles flight plan to the map
    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles

        PlanMapItems {
            map:                flightMap
            largeMapView:       _mainIsMap
            masterController:   masterController
            isActiveVehicle:    _vehicle.active

            property var _vehicle: object

            PlanMasterController {
                id: masterController
                Component.onCompleted: startStaticActiveVehicle(object)
            }
        }
    }

    // Allow custom builds to add map items
    CustomMapItems {
        map:            flightMap
        largeMapView:   _mainIsMap
    }

    GeoFenceMapVisuals {
        map:                    flightMap
        myGeoFenceController:   _geoFenceController
        interactive:            false
        planView:               false
        homePosition:           _activeVehicle && _activeVehicle.homePosition.isValid ? _activeVehicle.homePosition : undefined
    }

    // Rally points on map
    MapItemView {
        model: _rallyPointController.points

        delegate: MapQuickItem {
            id:             itemIndicator
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            coordinate:     object.coordinate
            z:              QGroundControl.zOrderMapItems

            sourceItem: MissionItemIndexLabel {
                id:         itemIndexLabel
                label:      qsTr("R", "rally point map item label")
            }
        }
    }

    // GoTo here waypoint
    MapQuickItem {
        coordinate:     _gotoHereCoordinate
        visible:        _activeVehicle && _activeVehicle.guidedModeSupported && _gotoHereCoordinate.isValid
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY

        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("Goto here", "Goto here waypoint")
        }
    }    

    // Camera trigger points
    MapItemView {
        model: _activeVehicle ? _activeVehicle.cameraTriggerPoints : 0

        delegate: CameraTriggerIndicator {
            coordinate:     object.coordinate
            z:              QGroundControl.zOrderTopMost
        }
    }

    // Handle guided mode clicks
    MouseArea {
        anchors.fill: parent

        onClicked: {
            if (guidedActionsController.showGotoLocation && !guidedActionsController.guidedUIVisible) {
                _gotoHereCoordinate = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                guidedActionsController.confirmAction(guidedActionsController.actionGoto, _gotoHereCoordinate)
            }
        }
    }

    MapScale {
        id:                     mapScale
        anchors.right:          parent.right
        anchors.margins:        ScreenTools.defaultFontPixelHeight * (0.33)
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * (0.33) + state === "bottomMode" ? 0 : ScreenTools.toolbarHeight
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * (0.33)
        mapControl:             flightMap
        visible:                !ScreenTools.isTinyScreen
        state:                  "bottomMode"
        states: [
            State {
                name:   "topMode"
                AnchorChanges {
                    target:                 mapScale
                    anchors.top:            parent.top
                    anchors.bottom:         undefined
                }
            },
            State {
                name:   "bottomMode"
                AnchorChanges {
                    target:                 mapScale
                    anchors.top:            undefined
                    anchors.bottom:         parent.bottom
                }
            }
        ]
    }
}
