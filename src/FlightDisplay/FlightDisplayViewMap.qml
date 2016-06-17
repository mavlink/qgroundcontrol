/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.4
import QtQuick.Controls             1.3
import QtLocation                   5.3
import QtPositioning                5.2

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

    property alias  missionController: _missionController
    property var    flightWidgets

    property bool   _followVehicle:                 true
    property var    _activeVehicle:                 QGroundControl.multiVehicleManager.activeVehicle
    property bool   _activeVehicleCoordinateValid:  _activeVehicle ? _activeVehicle.coordinateValid : false
    property var    activeVehicleCoordinate:        _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property var    _gotoHereCoordinate:            QtPositioning.coordinate()
    property int    _retaskSequence:                0

    Component.onCompleted: {
        QGroundControl.flightMapPosition = center
        QGroundControl.flightMapZoom = zoomLevel
    }
    onCenterChanged: QGroundControl.flightMapPosition = center
    onZoomLevelChanged: QGroundControl.flightMapZoom = zoomLevel

    onActiveVehicleCoordinateChanged: {
        if (_followVehicle && _activeVehicleCoordinateValid && activeVehicleCoordinate.isValid) {
            _initialMapPositionSet = true
            flightMap.center  = activeVehicleCoordinate
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    MissionController {
        id: _missionController
        Component.onCompleted: start(false /* editMode */)
    }

    // Add trajectory points to the map
    MapItemView {
        model: _mainIsMap ? _activeVehicle ? _activeVehicle.trajectoryPoints : 0 : 0
        delegate:
            MapPolyline {
            line.width: 3
            line.color: "red"
            z:          QGroundControl.zOrderMapItems - 1
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
            size:           _mainIsMap ? ScreenTools.defaultFontPixelHeight * 5 : ScreenTools.defaultFontPixelHeight * 2
            z:              QGroundControl.zOrderMapItems
        }
    }

    // Add the mission items to the map
    MissionItemView {
        model: _mainIsMap ? _missionController.visualItems : 0
    }

    // Add lines between waypoints
    MissionLineView {
        model: _mainIsMap ? _missionController.waypointLines : 0
    }

    // GoTo here waypoint
    MapQuickItem {
        coordinate:     _gotoHereCoordinate
        visible:        _activeVehicle && _activeVehicle.guidedMode && _gotoHereCoordinate.isValid
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2

        sourceItem: MissionItemIndexLabel {
            isCurrentItem:  true
            label:          qsTr("G", "Goto here waypoint") // second string is translator's hint.
        }
    }    

    MapScale {
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * (0.66)
        anchors.rightMargin:    ScreenTools.defaultFontPixelHeight * (0.33)
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        z:                      QGroundControl.zOrderWidgets
        mapControl:             flightMap
        visible:                !ScreenTools.isTinyScreen
    }

    // Handle guided mode clicks
    MouseArea {
        anchors.fill: parent

        onClicked: {
            if (_activeVehicle) {
                if (_activeVehicle.guidedMode && flightWidgets.guidedModeBar.state == "Shown") {
                    _gotoHereCoordinate = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                    flightWidgets.guidedModeBar.confirmAction(flightWidgets.guidedModeBar.confirmGoTo)
                } else {
                    flightWidgets.guidedModeBar.state = "Shown"
                }
            }
        }
    }
}
