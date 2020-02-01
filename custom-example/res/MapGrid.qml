/****************************************************************************
 *
 *   (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

Item {
    id: _root
    visible: mainIsMap && QGroundControl.settingsManager.appSettings.displayMGRSCoordinates.rawValue

    readonly property string _mainIsMapKey: "MainFlyWindowIsMap"
    property bool videoOnSecondScreen: Qt.application.screens.length > 1 && QGroundControl.settingsManager.videoSettings.showVideoOnSecondScreen.value
    property bool mainIsMap: QGroundControl.videoManager.hasVideo && !videoOnSecondScreen ? QGroundControl.loadBoolGlobalSetting(_mainIsMapKey,  true) : true
    property var mapControl
    property var centerViewport: (mapControl) ? mapControl.centerViewport : null
    property var viewportGeomerty: (centerViewport) ? centerViewport.width * centerViewport.height : 0
    property var mapGridObject
    property var polylineComponents: []
    property var polylines: []

    onMapControlChanged: {
        if (mapControl) {
        }
    }

    Connections {
        target:             mapControl
        onWidthChanged:     geometryTimer.restart()
        onHeightChanged:    geometryTimer.restart()
        onZoomLevelChanged: geometryTimer.restart()
        onCenterChanged:    geometryTimer.restart()
    }

    Timer {
        id:                 geometryTimer
        interval:           100
        running:            false
        repeat:             false
        onTriggered:        geometryChanged()
    }

    onPolylinesChanged: {
        if (mapControl && centerViewport) {
            addVisuals()
        }
    }

    function geometryChanged() {
        if (centerViewport) {
            var rect = Qt.rect(centerViewport.x, centerViewport.y, centerViewport.width, centerViewport.height)
            var topLeftCoord = mapControl.toCoordinate(Qt.point(rect.x, rect.y), false /* clipToViewPort */)
            var bottomRightCoord = mapControl.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height), false /* clipToViewPort */)
            if (mapGridObject) {
                mapGridObject.geometryChanged(mapControl.zoomLevel, topLeftCoord, bottomRightCoord)
            }
            addVisuals()
        }
    }

    function addVisuals() {
        removeVisuals()
        for (var i = 0; i < polylines.length; i++) {
            var pc = polylineComponent.createObject(mapControl)
            if (pc) {
                var pl = polylines[i]
                pc.line.width = pl.width
                pc.line.color = pl.color
                for (var j = 0; j < pl.points.length; j++) {
                    pc.addCoordinate(QtPositioning.coordinate(pl.points[j].lat, pl.points[j].lng))
                }
                mapControl.addMapItem(pc)
                polylineComponents.push(pc)
            }
        }
    }

    function removeVisuals() {
        for (var i = 0; i < polylineComponents.length; i++) {
            polylineComponents[i].destroy()
        }
        polylineComponents = []
    }

    Component {
        id: polylineComponent
        MapPolyline {
            visible: _root.visible
        }
    }
}
