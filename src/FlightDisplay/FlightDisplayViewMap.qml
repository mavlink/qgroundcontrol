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

    QGCMapLabel {
        id:                         flyLabel
        map:                        flightMap
        text:                       qsTr("Fly")
        visible:                    !ScreenTools.isShortScreen
        anchors.topMargin:          _toolButtonTopMargin
        anchors.horizontalCenter:   centerMapDropButton.horizontalCenter
        anchors.top:                parent.top
    }

    //-- Vertical Tool Buttons

    ExclusiveGroup {
        id: dropButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: mapTypeButtonsExclusiveGroup
    }

    //-- Dismiss Drop Down (if any)
    MouseArea {
        anchors.fill:   parent
        enabled:        dropButtonsExclusiveGroup.current != null
        onClicked: {
            if (dropButtonsExclusiveGroup.current) {
                dropButtonsExclusiveGroup.current.checked = false
            }
            dropButtonsExclusiveGroup.current = null
        }
    }

    // IMPORTANT NOTE: Drop Buttons must be parented directly to the map. If they are placed in a Column for example the drop control positioning
    // will not work correctly.

    //-- Map Center Control
    CenterMapDropButton {
        id:                     centerMapDropButton
        anchors.topMargin:      flyLabel.visible ? ScreenTools.defaultFontPixelHeight / 2 : _toolButtonTopMargin
        anchors.leftMargin:     ScreenTools.defaultFontPixelHeight
        anchors.left:           parent.left
        anchors.top:            flyLabel.visible ? flyLabel.bottom : parent.top
        z:                      QGroundControl.zOrderWidgets
        exclusiveGroup:         dropButtonsExclusiveGroup
        map:                    _flightMap
        mapFitViewport:         Qt.rect(leftToolWidth, _toolButtonTopMargin, flightMap.width - leftToolWidth - rightPanelWidth, flightMap.height - _toolButtonTopMargin)
        usePlannedHomePosition: false
        geoFenceController:     geoFenceController
        missionController:      missionController
        rallyPointController:   rallyPointController
        showFollowVehicle:      true
        followVehicle:          _followVehicle
        onFollowVehicleChanged: _followVehicle = followVehicle

        property real leftToolWidth:    centerMapDropButton.x + centerMapDropButton.width
    }

    //-- Map Type Control
    DropButton {
        id:                 mapTypeButton
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight
        anchors.top:        centerMapDropButton.bottom
        anchors.left:       centerMapDropButton.left
        dropDirection:      dropRight
        buttonImage:        "/qmlimages/MapType.svg"
        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
        exclusiveGroup:     dropButtonsExclusiveGroup
        z:                  QGroundControl.zOrderWidgets
        lightBorders:       isSatelliteMap

        dropDownComponent: Component {
            Column {
                spacing: ScreenTools.defaultFontPixelWidth

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Repeater {
                        model: QGroundControl.flightMapSettings.mapTypes

                        QGCButton {
                            checkable:      true
                            checked:        QGroundControl.flightMapSettings.mapType === text
                            text:           modelData
                            width:          clearButton.width
                            exclusiveGroup: mapTypeButtonsExclusiveGroup

                            onClicked: {
                                QGroundControl.flightMapSettings.mapType = text
                                checked = true
                                dropButtonsExclusiveGroup.current = null
                            }
                        }
                    }
                }

                QGCButton {
                    id:         clearButton
                    text:       qsTr("Clear Flight Trails")
                    enabled:    QGroundControl.multiVehicleManager.activeVehicle
                    onClicked: {
                        QGroundControl.multiVehicleManager.activeVehicle.clearTrajectoryPoints()
                        dropButtonsExclusiveGroup.current = null
                    }
                }
            }
        }
    }

    //-- Zoom Map In
    RoundButton {
        id:                 mapZoomPlus
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight
        anchors.top:        mapTypeButton.bottom
        anchors.left:       mapTypeButton.left
        visible:            !ScreenTools.isTinyScreen && _mainIsMap
        buttonImage:        "/qmlimages/ZoomPlus.svg"
        exclusiveGroup:     dropButtonsExclusiveGroup
        z:                  QGroundControl.zOrderWidgets
        lightBorders:       isSatelliteMap
        onClicked: {
            if(_flightMap)
                _flightMap.zoomLevel += 0.5
            checked = false
        }
    }

    //-- Zoom Map Out
    RoundButton {
        id:                 mapZoomMinus
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight
        anchors.top:        mapZoomPlus.bottom
        anchors.left:       mapZoomPlus.left
        visible:            !ScreenTools.isTinyScreen && _mainIsMap
        buttonImage:        "/qmlimages/ZoomMinus.svg"
        exclusiveGroup:     dropButtonsExclusiveGroup
        z:                  QGroundControl.zOrderWidgets
        lightBorders:       isSatelliteMap
        onClicked: {
            if(_flightMap)
                _flightMap.zoomLevel -= 0.5
            checked = false
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
            size:           _mainIsMap ? ScreenTools.defaultFontPixelHeight * 5 : ScreenTools.defaultFontPixelHeight * 2
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
        anchorPoint:    Qt.point(sourceItem.width / 2, sourceItem.height / 2)
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
            anchorPoint:    Qt.point(sourceItem.width / 2, sourceItem.height / 2)
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
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2

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
