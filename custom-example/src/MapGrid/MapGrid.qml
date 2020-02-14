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

    property bool enabled: QGroundControl.settingsManager.appSettings.displayMGRSCoordinates.rawValue
    property var mapControl: null
    property var centerViewport: (mapControl) ? mapControl.centerViewport : null
    property var viewportGeomerty: (centerViewport) ? centerViewport.width * centerViewport.height : 0
    property var mapComponents: []
    property var componentsToAdd: []
    property var values: mapGrid.values
    property double startTime: 0
    property int maxTimePerStepMs: 50
    property bool timeMeasured: false
    property int maxNumberOfComponentsPerStep: 100

    MapGrid {
        id: mapGrid
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
        interval:           500
        running:            false
        repeat:             false
        onTriggered:        geometryChanged()
    }

    Timer {
        id:                 addComponentsTimer
        interval:           maxTimePerStepMs
        running:            false
        repeat:             false
        onTriggered:        addComponents()
    }

    onEnabledChanged: {
        geometryChanged()
    }

    onMapControlChanged: {
        geometryChanged()
    }

    onValuesChanged: {
        addVisuals()
    }

    function geometryChanged() {
        if (!mainIsMap || !enabled) {
            removeVisuals()
            return
        }

        if (mapControl && centerViewport) {
            var rect = Qt.rect(centerViewport.x, centerViewport.y, centerViewport.width, centerViewport.height)
            var topLeftCoord = mapControl.toCoordinate(Qt.point(rect.x, rect.y), false /* clipToViewPort */)
            var bottomRightCoord = mapControl.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height), false /* clipToViewPort */)
            mapGrid.geometryChanged(mapControl.zoomLevel, topLeftCoord, bottomRightCoord)
        }
    }

    function addVisuals() {
        if (!mapControl || !centerViewport || !values || !values.hasOwnProperty("lines")) {
            return;
        }

        if (!timeMeasured) {
            startTime = new Date().getTime()
        }

        removeVisuals()

//        console.info("MapGrid.qml - adding lines: " + values.lines.length)
        for (var i = 0; i < values.lines.length; i++) {
            var pc = polylineComponent.createObject(mapControl)
            if (pc) {
                pc.visible = true
                var pl = values.lines[i]
                pc.line.width = pl.width
                pc.line.color = pl.color
                for (var j = 0; j < pl.points.length; j++) {
                    pc.addCoordinate(QtPositioning.coordinate(pl.points[j].lat, pl.points[j].lng))
                }

                componentsToAdd.push(pc)
            }
        }

        if (!values.hasOwnProperty("labels")) {
            return;
        }

//        console.info("MapGrid.qml - adding labels: " + values.labels.length)
        for (i = 0; i < values.labels.length; i++) {
            var lc = labelComponent.createObject(mapControl)
            if (lc) {
                lc.visible = true
                var l = values.labels[i]
                lc.labelText = l.text
                lc.coordinate = QtPositioning.coordinate(l.lat, l.lng)
                lc.backgroundColor = l.backgroundColor
                lc.foregroundColor = l.foregroundColor

                componentsToAdd.push(lc)
            }
        }

        addComponents()
    }

    function removeVisuals() {
        for (var i = 0; i < mapComponents.length; i++) {
            mapComponents[i].destroy()
        }
        mapComponents = []
    }

    function addComponents() {
        for (var count = 0; count < maxNumberOfComponentsPerStep && componentsToAdd.length > 0; count++) {
            var c = componentsToAdd.pop()
            mapControl.addMapItem(c)
            mapComponents.push(c)
        }

        if (componentsToAdd.length > 0) {
            if (!timeMeasured) {
                var dt = new Date().getTime() - startTime
                maxNumberOfComponentsPerStep = maxNumberOfComponentsPerStep / dt * maxTimePerStepMs;
//                console.info("Max number of components: " + maxNumberOfComponentsPerStep)
                timeMeasured = true;
            }

            addComponentsTimer.restart()
        }

    }

    Component {
        id: polylineComponent
        MapPolyline {
            visible: _root.visible
            z: 0
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
                    border.width:           1
                    border.color:           foregroundColor
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

