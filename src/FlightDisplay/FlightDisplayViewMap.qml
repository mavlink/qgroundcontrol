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
    id:             flightMap
    anchors.fill:   parent
    mapName:        _mapName

    property alias  missionController: missionController
    property var    flightWidgets
    property var    rightPanelWidth
    property var    qgcView             ///< QGCView control which contains this map

    property var    _activeVehicle:                 QGroundControl.multiVehicleManager.activeVehicle
    property var    _activeVehicleCoordinate:       _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property var    _gotoHereCoordinate:            QtPositioning.coordinate()
    property int    _retaskSequence:                0
    property real   _toolButtonTopMargin:           parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)

    property bool   _disableVehicleTracking:        false
    property bool   _keepVehicleCentered:           _mainIsMap ? false : true
    property bool   _firstVehiclePositionReceived:  false
    property bool   _userPanned:                    false

    Component.onCompleted: {
        QGroundControl.flightMapPosition = center
        QGroundControl.flightMapZoom = zoomLevel
        possibleCenterToGCSPosition()
    }

    // Track last known map position and zoom in settings
    onZoomLevelChanged: QGroundControl.flightMapZoom = zoomLevel
    onCenterChanged:    QGroundControl.flightMapPosition = center

    // We move the map to the gcs position id:
    //  - We don't have a vehicle position yet
    //  - The user has not futzed with the map
    onGcsPositionChanged: possibleCenterToGCSPosition()

    function possibleCenterToGCSPosition() {
        if (!_firstVehiclePositionReceived && !_userPanned && gcsPosition.isValid) {
            center = gcsPosition
        }
    }

    // When the user pans the map we stop responding to vehicle coordinate updates until the panRecenterTimer fires
    Connections {
        target: gesture

        onPanFinished: {
            _userPanned = true
            _disableVehicleTracking = true
            panRecenterTimer.start()
        }

        onFlickFinished: {
            _userPanned = true
            _disableVehicleTracking = true
            panRecenterTimer.start()
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
        if (_activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            if (_keepVehicleCentered) {
                _firstVehiclePositionReceived = true
                flightMap.center = _activeVehicleCoordinate
            } else {
                if (!_firstVehiclePositionReceived) {
                    _firstVehiclePositionReceived = true
                    flightMap.center = _activeVehicleCoordinate
                } else if (recenterNeeded()) {
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

    MissionController {
        id: missionController
        Component.onCompleted: start(false /* editMode */)
    }

    GeoFenceController {
        id: geoFenceController
        Component.onCompleted: start(false /* editMode */)
    }

    RallyPointController {
        id: rallyPointController
        Component.onCompleted: start(false /* editMode */)
    }

    // The following code is used to track vehicle states such that we prompt to remove mission from vehicle when mission completes

    property bool vehicleArmed:                 _activeVehicle ? _activeVehicle.armed : false
    property bool vehicleWasArmed:              false
    property bool vehicleInMissionFlightMode:   _activeVehicle ? (_activeVehicle.flightMode === _activeVehicle.missionFlightMode) : false
    property bool promptForMissionRemove:       false

    onVehicleArmedChanged: {
        if (vehicleArmed) {
            if (!promptForMissionRemove) {
                promptForMissionRemove = vehicleInMissionFlightMode
                vehicleWasArmed = true
            }
        } else {
            if (promptForMissionRemove && (missionController.containsItems || geoFenceController.containsItems || rallyPointController.containsItems)) {
                qgcView.showDialog(removeMissionDialogComponent, qsTr("Flight complete"), showDialogDefaultWidth, StandardButton.No | StandardButton.Yes)
            }
            promptForMissionRemove = false
        }
    }

    onVehicleInMissionFlightModeChanged: {
        if (!promptForMissionRemove && vehicleArmed) {
            promptForMissionRemove = true
        }
    }

    Component {
        id: removeMissionDialogComponent

        QGCViewMessage {
            message: qsTr("Do you want to remove the mission from the vehicle?")

            function accept() {
                missionController.removeAllFromVehicle()
                geoFenceController.removeAllFromVehicle()
                rallyPointController.removeAllFromVehicle()
                hideDialog()

            }
        }
    }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    ToolStrip {
        id:                 toolStrip
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.topMargin:  _toolButtonTopMargin
        anchors.top:        parent.top
        color:              qgcPal.window
        title:              qsTr("Fly")
        z:                  QGroundControl.zOrderWidgets
        buttonVisible:      [ true, _showZoom, _showZoom ]
        maxHeight:          (_flightVideo.visible ? _flightVideo.y : parent.height) - toolStrip.y   // Massive reach across hack

        property bool _showZoom: !ScreenTools.isMobile

        model: [
            {
                name:               "Center",
                iconSource:         "/qmlimages/MapCenter.svg",
                dropPanelComponent: centerMapDropPanel
            },
            {
                name:               "In",
                iconSource:         "/qmlimages/ZoomPlus.svg"
            },
            {
                name:               "Out",
                iconSource:         "/qmlimages/ZoomMinus.svg"
            }
        ]

        onClicked: {
            switch (index) {
            case 1:
                _flightMap.zoomLevel += 0.5
                break
            case 2:
                _flightMap.zoomLevel -= 0.5
                break
            }
        }
    }

    // Toolstrip drop panel compomnents

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        _flightMap
        mapFitViewport:             Qt.rect(leftToolWidth, _toolButtonTopMargin, flightMap.width - leftToolWidth - rightPanelWidth, flightMap.height - _toolButtonTopMargin)
        usePlannedHomePosition:     false
        mapMissionController:      missionController
        mapGeoFenceController:     geoFenceController
        mapRallyPointController:   rallyPointController

        property real leftToolWidth:    toolStrip.x + toolStrip.width
    }

    Component {
        id: centerMapDropPanel

        CenterMapDropPanel {
            map:                _flightMap
            fitFunctions:       mapFitFunctions
        }
    }

    // Add trajectory points to the map
    MapItemView {
        model: _mainIsMap ? _activeVehicle ? _activeVehicle.trajectoryPoints : 0 : 0
        delegate:
            MapPolyline {
            line.width: 3
            line.color: "red"
            z:          QGroundControl.zOrderMapItems - 2
            path: [
                object.coordinate1,
                object.coordinate2,
            ]
        }
    }

    // Add the vehicles to the map
    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate:
            VehicleMapItem {
            vehicle:        object
            coordinate:     object.coordinate
            isSatellite:    flightMap.isSatelliteMap
            size:           _mainIsMap ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight
            z:              QGroundControl.zOrderMapItems - 1
        }
    }

    // Add the mission item visuals to the map
    Repeater {
        model: _mainIsMap ? missionController.visualItems : 0

        delegate: MissionItemMapVisual {
            map:        flightMap

            onClicked: {
                _retaskSequence = object.sequenceNumber
                flightWidgets.guidedModeBar.confirmAction(parent.flightWidgets.guidedModeBar.confirmRetask)
            }
        }
    }

    // Add lines between waypoints
    MissionLineView {
        model: _mainIsMap ? missionController.waypointLines : 0
    }

    GeoFenceMapVisuals {
        map:                    flightMap
        myGeoFenceController:   geoFenceController
        interactive:            false
        homePosition:           _activeVehicle && _activeVehicle.homePositionAvailable ? _activeVehicle.homePosition : undefined
    }

    // Rally points on map
    MapItemView {
        model: rallyPointController.points

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
        visible:        _activeVehicle && _activeVehicle.guidedMode && _gotoHereCoordinate.isValid
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY

        sourceItem: MissionItemIndexLabel {
            checked: true
            label:   qsTr("G", "Goto here waypoint") // second string is translator's hint.
        }
    }    

    MapScale {
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * (0.66)
        anchors.rightMargin:    ScreenTools.defaultFontPixelHeight * (0.33)
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        mapControl:             flightMap
        visible:                !ScreenTools.isTinyScreen
    }

    // Handle guided mode clicks
    MouseArea {
        anchors.fill: parent

        onClicked: {
            if (_activeVehicle) {
                if (flightWidgets.guidedModeBar.state != "Shown") {
                    flightWidgets.guidedModeBar.state = "Shown"
                } else {
                    if (flightWidgets.gotoEnabled) {
                        _gotoHereCoordinate = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                        flightWidgets.guidedModeBar.confirmAction(flightWidgets.guidedModeBar.confirmGoTo)
                    }
                }
            }
        }
    }
}
