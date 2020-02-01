/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtLocation                   5.3
import QtPositioning                5.3
import QtQuick.Dialogs              1.2

import QGroundControl               1.0
import QGroundControl.Airspace      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

FlightMap {
    id:                         flightMap
    mapName:                    _mapName
    allowGCSLocationCenter:     !userPanned
    allowVehicleLocationCenter: !_keepVehicleCentered
    planView:                   false
    zoomLevel:                  QGroundControl.flightMapZoom
    center:                     QGroundControl.flightMapPosition

    property alias  scaleState: mapScale.state

    // The following properties must be set by the consumer
    property var    guidedActionsController
    property var    flightWidgets
    property var    rightPanelWidth
    property var    multiVehicleView                    ///< true: multi-vehicle view, false: single vehicle view
    property var    missionController:          null

    property rect   centerViewport:             Qt.rect(0, 0, width, height)

    property var    _geoFenceController:        missionController.geoFenceController
    property var    _rallyPointController:      missionController.rallyPointController
    property var    _activeVehicleCoordinate:   activeVehicle ? activeVehicle.coordinate : QtPositioning.coordinate()
    property real   _toolButtonTopMargin:       parent.height - mainWindow.height + (ScreenTools.defaultFontPixelHeight / 2)
    property bool   _airspaceEnabled:           QGroundControl.airmapSupported ? (QGroundControl.settingsManager.airMapSettings.enableAirMap.rawValue && QGroundControl.airspaceManager.connected): false
    property var    _flyViewSettings:           QGroundControl.settingsManager.flyViewSettings
    property bool   _keepMapCenteredOnVehicle:  _flyViewSettings.keepMapCenteredOnVehicle.rawValue

    property bool   _disableVehicleTracking:    false
    property bool   _keepVehicleCentered:       mainIsMap ? false : true
    property bool   _pipping:                   false

    function updateAirspace(reset) {
        if(_airspaceEnabled) {
            var coordinateNW = flightMap.toCoordinate(Qt.point(0,0), false /* clipToViewPort */)
            var coordinateSE = flightMap.toCoordinate(Qt.point(width,height), false /* clipToViewPort */)
            if(coordinateNW.isValid && coordinateSE.isValid) {
                QGroundControl.airspaceManager.setROI(coordinateNW, coordinateSE, false /*planView*/, reset)
            }
        }
    }

    function pipIn() {
        if(QGroundControl.flightMapZoom > 3) {
            _pipping = true;
            zoomLevel = QGroundControl.flightMapZoom - 3
            _pipping = false;
        }
    }

    function pipOut() {
        _pipping = true;
        zoomLevel = QGroundControl.flightMapZoom
        _pipping = false;
    }

    function adjustMapSize() {
        if(mainIsMap)
            pipOut()
        else
            pipIn()
    }

    // Track last known map position and zoom from Fly view in settings

    onVisibleChanged: {
        if(visible) {
            adjustMapSize()
            center    = QGroundControl.flightMapPosition
        }
    }

    onZoomLevelChanged: {
        if(!_pipping) {
            QGroundControl.flightMapZoom = zoomLevel
            updateAirspace(false)
        }
    }
    onCenterChanged: {
        QGroundControl.flightMapPosition = center
        updateAirspace(false)
    }

    // When the user pans the map we stop responding to vehicle coordinate updates until the panRecenterTimer fires
    onUserPannedChanged: {
        if (userPanned) {
            userPanned = false
            _disableVehicleTracking = true
            panRecenterTimer.restart()
        }
    }

    on_AirspaceEnabledChanged: {
        updateAirspace(true)
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
        var toolStripRightEdge = mapFromItem(toolStrip, toolStrip.x, 0).x + toolStrip.width
        var instrumentsWidth = 0
        if (QGroundControl.corePlugin.options.instrumentWidget && QGroundControl.corePlugin.options.instrumentWidget.widgetPosition === CustomInstrumentWidget.POS_TOP_RIGHT) {
            // Assume standard instruments
            instrumentsWidth = flightDisplayViewWidgets.getPreferredInstrumentWidth()
        }
        var centerViewport = Qt.rect(toolStripRightEdge, 0, width - toolStripRightEdge - instrumentsWidth, height)
        return !pointInRect(vehiclePoint, centerViewport)
    }

    function updateMapToVehiclePosition() {
        // We let FlightMap handle first vehicle position
        if (!_keepMapCenteredOnVehicle && firstVehiclePositionReceived && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            if (_keepVehicleCentered) {
                flightMap.center = _activeVehicleCoordinate
            } else {
                if (firstVehiclePositionReceived && recenterNeeded()) {
                    animatedMapRecenter(flightMap.center, _activeVehicleCoordinate)
                }
            }
        }
    }

    on_ActiveVehicleCoordinateChanged: {
        if (_keepMapCenteredOnVehicle && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            flightMap.center = _activeVehicleCoordinate
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

    QGCMapPalette { id: mapPal; lightColors: isSatelliteMap }

    Connections {
        target:                 missionController
        ignoreUnknownSignals:   true
        onNewItemsFromVehicle: {
            var visualItems = missionController.visualItems
            if (visualItems && visualItems.count !== 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
                firstVehiclePositionReceived = true
            }
        }
    }

    MapFitFunctions {
        id:                         mapFitFunctions // The name for this id cannot be changed without breaking references outside of this code. Beware!
        map:                        mainWindow.flightDisplayMap
        usePlannedHomePosition:     false
        planMasterController:       missionController
        property real leftToolWidth: toolStrip.x + toolStrip.width
    }

    // Add trajectory lines to the map
    MapPolyline {
        id:         trajectoryPolyline
        line.width: 3
        line.color: "red"
        z:          QGroundControl.zOrderTrajectoryLines
        visible:    mainIsMap

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: trajectoryPolyline.path = activeVehicle ? activeVehicle.trajectoryPoints.list() : []
        }

        Connections {
            target:                 activeVehicle ? activeVehicle.trajectoryPoints : null
            onPointAdded:           trajectoryPolyline.addCoordinate(coordinate)
            onUpdateLastPoint:      trajectoryPolyline.replaceCoordinate(trajectoryPolyline.pathLength() - 1, coordinate)
            onPointsCleared:        trajectoryPolyline.path = []
        }
    }

    // Add the vehicles to the map
    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: VehicleMapItem {
            vehicle:        object
            coordinate:     object.coordinate
            map:            flightMap
            size:           mainIsMap ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight
            z:              QGroundControl.zOrderVehicles
        }
    }

    // Add ADSB vehicles to the map
    MapItemView {
        model: QGroundControl.adsbVehicleManager.adsbVehicles
        delegate: VehicleMapItem {
            coordinate:     object.coordinate
            altitude:       object.altitude
            callsign:       object.callsign
            heading:        object.heading
            alert:          object.alert
            map:            flightMap
            z:              QGroundControl.zOrderVehicles
        }
    }

    // Add the items associated with each vehicles flight plan to the map
    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles

        PlanMapItems {
            map:                flightMap
            largeMapView:       mainIsMap
            masterController:   masterController
            vehicle:            _vehicle

            property var _vehicle: object

            PlanMasterController {
                id: masterController
                Component.onCompleted: startStaticActiveVehicle(object)
            }
        }
    }

    MapItemView {
        model: mainIsMap ? _missionController.directionArrows : undefined

        delegate: MapLineArrow {
            fromCoord:      object ? object.coordinate1 : undefined
            toCoord:        object ? object.coordinate2 : undefined
            arrowPosition:  2
            z:              QGroundControl.zOrderWaypointLines
        }
    }

    // Allow custom builds to add map items
    CustomMapItems {
        map:            flightMap
        largeMapView:   mainIsMap
    }

    GeoFenceMapVisuals {
        map:                    flightMap
        myGeoFenceController:   _geoFenceController
        interactive:            false
        planView:               false
        homePosition:           activeVehicle && activeVehicle.homePosition.isValid ? activeVehicle.homePosition :  QtPositioning.coordinate()
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

    // Camera trigger points
    MapItemView {
        model: activeVehicle ? activeVehicle.cameraTriggerPoints : 0

        delegate: CameraTriggerIndicator {
            coordinate:     object.coordinate
            z:              QGroundControl.zOrderTopMost
        }
    }

    // GoTo Location visuals
    MapQuickItem {
        id:             gotoLocationItem
        visible:        false
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("Go here", "Go to location waypoint")
        }

        property bool inGotoFlightMode: activeVehicle ? activeVehicle.flightMode === activeVehicle.gotoFlightMode : false

        onInGotoFlightModeChanged: {
            if (!inGotoFlightMode && gotoLocationItem.visible) {
                // Hide goto indicator when vehicle falls out of guided mode
                gotoLocationItem.visible = false
            }
        }

        Connections {
            target: mainWindow
            onActiveVehicleChanged: {
                if (!activeVehicle) {
                    gotoLocationItem.visible = false
                }
            }
        }

        function show(coord) {
            gotoLocationItem.coordinate = coord
            gotoLocationItem.visible = true
        }

        function hide() {
            gotoLocationItem.visible = false
        }

        function actionConfirmed() {
            // We leave the indicator visible. The handling for onInGuidedModeChanged will hide it.
        }

        function actionCancelled() {
            hide()
        }
    }

    // Orbit editing visuals
    QGCMapCircleVisuals {
        id:             orbitMapCircle
        mapControl:     parent
        mapCircle:      _mapCircle
        visible:        false

        property alias center:              _mapCircle.center
        property alias clockwiseRotation:   _mapCircle.clockwiseRotation
        readonly property real defaultRadius: 30

        Connections {
            target: mainWindow
            onActiveVehicleChanged: {
                if (!activeVehicle) {
                    orbitMapCircle.visible = false
                }
            }
        }

        function show(coord) {
            _mapCircle.radius.rawValue = defaultRadius
            orbitMapCircle.center = coord
            orbitMapCircle.visible = true
        }

        function hide() {
            orbitMapCircle.visible = false
        }

        function actionConfirmed() {
            // Live orbit status is handled by telemetry so we hide here and telemetry will show again.
            hide()
        }

        function actionCancelled() {
            hide()
        }

        function radius() {
            return _mapCircle.radius.rawValue
        }

        Component.onCompleted: guidedActionsController.orbitMapCircle = orbitMapCircle

        QGCMapCircle {
            id:                 _mapCircle
            interactive:        true
            radius.rawValue:    30
            showRotation:       true
            clockwiseRotation:  true
        }
    }

    // ROI Location visuals
    MapQuickItem {
        id:             roiLocationItem
        visible:        activeVehicle && activeVehicle.isROIEnabled
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("ROI here", "Make this a Region Of Interest")
        }

        //-- Visibilty controlled by actual state
        function show(coord) {
            roiLocationItem.coordinate = coord
        }

        function hide() {
        }

        function actionConfirmed() {
        }

        function actionCancelled() {
        }
    }

    // Orbit telemetry visuals
    QGCMapCircleVisuals {
        id:             orbitTelemetryCircle
        mapControl:     parent
        mapCircle:      activeVehicle ? activeVehicle.orbitMapCircle : null
        visible:        activeVehicle ? activeVehicle.orbitActive : false
    }

    MapQuickItem {
        id:             orbitCenterIndicator
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        coordinate:     activeVehicle ? activeVehicle.orbitMapCircle.center : QtPositioning.coordinate()
        visible:        orbitTelemetryCircle.visible

        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("Orbit", "Orbit waypoint")
        }
    }

    // Handle guided mode clicks
    MouseArea {
        anchors.fill: parent

        QGCMenu {
            id: clickMenu
            property var coord
            QGCMenuItem {
                text:           qsTr("Go to location")
                visible:        guidedActionsController.showGotoLocation

                onTriggered: {
                    gotoLocationItem.show(clickMenu.coord)
                    orbitMapCircle.hide()
                    guidedActionsController.confirmAction(guidedActionsController.actionGoto, clickMenu.coord, gotoLocationItem)
                }
            }
            QGCMenuItem {
                text:           qsTr("Orbit at location")
                visible:        guidedActionsController.showOrbit

                onTriggered: {
                    orbitMapCircle.show(clickMenu.coord)
                    gotoLocationItem.hide()
                    guidedActionsController.confirmAction(guidedActionsController.actionOrbit, clickMenu.coord, orbitMapCircle)
                }
            }
            QGCMenuItem {
                text:           qsTr("ROI at location")
                visible:        guidedActionsController.showROI

                onTriggered: {
                    roiLocationItem.show(clickMenu.coord)
                    guidedActionsController.confirmAction(guidedActionsController.actionROI, clickMenu.coord, orbitMapCircle)
                }
            }
        }

        onClicked: {
            if (guidedActionsController.guidedUIVisible || (!guidedActionsController.showGotoLocation && !guidedActionsController.showOrbit)) {
                return
            }
            orbitMapCircle.hide()
            gotoLocationItem.hide()
            var clickCoord = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
            if (guidedActionsController.showGotoLocation && guidedActionsController.showOrbit) {
                clickMenu.coord = clickCoord
                clickMenu.popup()
            } else if (guidedActionsController.showGotoLocation) {
                gotoLocationItem.show(clickCoord)
                guidedActionsController.confirmAction(guidedActionsController.actionGoto, clickCoord)
            } else if (guidedActionsController.showOrbit) {
                orbitMapCircle.show(clickCoord)
                guidedActionsController.confirmAction(guidedActionsController.actionOrbit, clickCoord)
            }
        }
    }

    MapScale {
        id:                     mapScale
        anchors.right:          parent.right
        anchors.margins:        _toolsMargin
        anchors.topMargin:      _toolsMargin + state === "bottomMode" ? 0 : ScreenTools.toolbarHeight
        mapControl:             flightMap
        buttonsOnLeft:          false
        visible:                !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.enableMapScale && mainIsMap
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

    // Airspace overlap support
    MapItemView {
        model:              _airspaceEnabled && QGroundControl.settingsManager.airMapSettings.enableAirspace && QGroundControl.airspaceManager.airspaceVisible ? QGroundControl.airspaceManager.airspaces.circles : []
        delegate: MapCircle {
            center:         object.center
            radius:         object.radius
            color:          object.color
            border.color:   object.lineColor
            border.width:   object.lineWidth
        }
    }

    MapItemView {
        model:              _airspaceEnabled && QGroundControl.settingsManager.airMapSettings.enableAirspace && QGroundControl.airspaceManager.airspaceVisible ? QGroundControl.airspaceManager.airspaces.polygons : []
        delegate: MapPolygon {
            path:           object.polygon
            color:          object.color
            border.color:   object.lineColor
            border.width:   object.lineWidth
        }
    }

}
