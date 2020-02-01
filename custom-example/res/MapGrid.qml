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
    property var mapComponents: []
    property var values: []

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

    onValuesChanged: {
        if (mapControl && centerViewport) {
            addVisuals()
        }
    }

    function geometryChanged() {
        if (centerViewport) {
            var rect = Qt.rect(centerViewport.x, centerViewport.y, centerViewport.width, centerViewport.height)
            var topLeftCoord = mapControl.toCoordinate(Qt.point(rect.x, rect.y), false /* clipToViewPort */)
            var topRightCoord = mapControl.toCoordinate(Qt.point(rect.x + rect.width, rect.y), false /* clipToViewPort */)
            var bottomLeftCoord = mapControl.toCoordinate(Qt.point(rect.x, rect.y + rect.height), false /* clipToViewPort */)
            var bottomRightCoord = mapControl.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height), false /* clipToViewPort */)
            if (mapGridObject) {
                mapGridObject.geometryChanged(mapControl.zoomLevel, topLeftCoord, topRightCoord, bottomLeftCoord, bottomRightCoord, centerViewport.width, centerViewport.height)
            }
            addVisuals()
        }
    }

    function addVisuals() {
        if (!values || !values.hasOwnProperty("lines")) {
            return;
        }

        removeVisuals()

        // Put other elements on top of the grid
        for (var n = 0; n < mapControl.mapItems.length; n++) {
            if (mapControl.mapItems[n].z < QGroundControl.zOrderMapItems) {
                mapControl.mapItems[n].z = QGroundControl.zOrderMapItems - 10;
            }
        }

//        console.info("MapGrid.qml - adding lines: " + values.lines.length)
        for (var i = 0; i < values.lines.length; i++) {
            var pc = polylineComponent.createObject(mapControl)
            if (pc) {
                var pl = values.lines[i]
                pc.line.width = pl.width
                pc.line.color = pl.color
                for (var j = 0; j < pl.points.length; j++) {
                    pc.addCoordinate(QtPositioning.coordinate(pl.points[j].lat, pl.points[j].lng))
                }
                mapControl.addMapItem(pc)
                mapComponents.push(pc)
            }
        }

        if (!values.hasOwnProperty("labels")) {
            return;
        }

        for (i = 0; i < values.labels.length; i++) {
            var lc = labelComponent.createObject(mapControl)
            if (lc) {
                var l = values.labels[i]
                lc.labelText = l.text
                lc.coordinate = QtPositioning.coordinate(l.lat, l.lng)
                lc.backgroundColor = l.backgroundColor
                lc.foregroundColor = l.foregroundColor

                mapControl.addMapItem(lc)
                mapComponents.push(lc)
            }
        }
    }

    function removeVisuals() {
        for (var i = 0; i < mapComponents.length; i++) {
            mapComponents[i].destroy()
        }
        mapComponents = []
    }

    Component {
        id: polylineComponent
        MapPolyline {
            visible: _root.visible
        }
    }

    Component {
        id: labelComponent
        MapQuickItem {
            anchorPoint.x: labelControl.width / 2
            anchorPoint.y: labelControl.height / 2
            z: 1
            visible: _root.visible

            property string labelText
            property color backgroundColor
            property color foregroundColor

            sourceItem: Canvas {
                Rectangle {
                    id:                     labelControl
                    anchors.leftMargin:     -4
                    anchors.rightMargin:    -4
                    anchors.fill:           labelControlLabel
                    color:                  backgroundColor
                }

                QGCLabel {
                    id:                     labelControlLabel
                    color:                  foregroundColor
                    text:                   labelText
                    visible:                labelControl.visible
                }
            }
        }
    }
}
