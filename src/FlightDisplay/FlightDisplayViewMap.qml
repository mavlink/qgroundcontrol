/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtLocation                   5.3
import QtPositioning                5.3

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

    property bool   _followVehicle:                 true
    property var    _activeVehicle:                 QGroundControl.multiVehicleManager.activeVehicle
    property bool   _activeVehicleCoordinateValid:  _activeVehicle ? _activeVehicle.coordinateValid : false
    property var    activeVehicleCoordinate:        _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property var    _gotoHereCoordinate:            QtPositioning.coordinate()
    property int    _retaskSequence:                0
    property real   _toolButtonTopMargin:           parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)

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
        buttonVisible:      [ true, true, _showZoom, _showZoom ]
        maxHeight:          (_flightVideo.visible ? _flightVideo.y : parent.height) - toolStrip.y   // Massive reach across hack

        property bool _showZoom: !ScreenTools.isMobile

        model: [
            {
                name:               "Center",
                iconSource:         "/qmlimages/MapCenter.svg",
                dropPanelComponent: centerMapDropPanel
            },
            {
                name:               "Map",
                iconSource:         "/qmlimages/MapType.svg",
                dropPanelComponent: mapTypeDropPanel
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
            case 2:
                _flightMap.zoomLevel += 0.5
                break
            case 3:
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
            showFollowVehicle:  true
            followVehicle:      _followVehicle

            onFollowVehicleChanged: _followVehicle = followVehicle
        }
    }

    Component {
        id: mapTypeDropPanel

        Column {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            QGCLabel { text: qsTr("Map type:") }
            Row {
                spacing: ScreenTools.defaultFontPixelWidth
                Repeater {
                    model: QGroundControl.flightMapSettings.mapTypes

                    QGCButton {
                        checkable:      true
                        checked:        QGroundControl.flightMapSettings.mapType === text
                        text:           modelData
                        exclusiveGroup: _mapTypeButtonsExclusiveGroup
                        onClicked: {
                            QGroundControl.flightMapSettings.mapType = text
                            dropPanel.hide()
                        }
                    }
                }
            }
        }
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
            size:           _mainIsMap ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight
            z:              QGroundControl.zOrderMapItems - 1
        }
    }

    // Add the mission items to the map
    MissionItemView {
        model: _mainIsMap ? missionController.visualItems : 0
    }

    // Add lines between waypoints
    MissionLineView {
        model: _mainIsMap ? missionController.waypointLines : 0
    }

    // GeoFence polygon
    MapPolygon {
        border.color:   "#80FF0000"
        border.width:   3
        path:           geoFenceController.polygon.path
        visible:        geoFenceController.polygonEnabled
    }

    // GeoFence circle
    MapCircle {
        border.color:   "#80FF0000"
        border.width:   3
        center:         missionController.plannedHomePosition
        radius:         geoFenceController.circleRadius
        z:              QGroundControl.zOrderMapItems
        visible:        geoFenceController.circleEnabled
    }

    // GeoFence breach return point
    MapQuickItem {
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        coordinate:     geoFenceController.breachReturnPoint
        visible:        geoFenceController.breachReturnEnabled
        sourceItem:     MissionItemIndexLabel { label: "F" }
        z:              QGroundControl.zOrderMapItems
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
                        _gotoHereCoordinate = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        flightWidgets.guidedModeBar.confirmAction(flightWidgets.guidedModeBar.confirmGoTo)
                    }
                }
            }
        }
    }
}
