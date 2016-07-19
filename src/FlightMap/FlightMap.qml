/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Map Background
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Vehicle               1.0
import QGroundControl.Mavlink               1.0

Map {
    id: _map

    property string mapName:            'defaultMap'
    property bool   isSatelliteMap:     activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

    readonly property real  maxZoomLevel: 20
    property variant        scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]

    function formatDistance(meters)
    {
        var dist = Math.round(meters)
        if (dist > 1000 ){
            if (dist > 100000){
                dist = Math.round(dist / 1000)
            }
            else{
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + " km"
        }
        else{
            dist = dist + " m"
        }
        return dist
    }

    function calculateScale() {
        var coord1, coord2, dist, text, f
        f = 0
        coord1 = _map.toCoordinate(Qt.point(0, scale.y))
        coord2 = _map.toCoordinate(Qt.point(0 + scaleImage.sourceSize.width, scale.y))
        dist = Math.round(coord1.distanceTo(coord2))
        if (dist === 0) {
            // not visible
        } else {
            for (var i = 0; i < scaleLengths.length - 1; i++) {
                if (dist < (scaleLengths[i] + scaleLengths[i+1]) / 2 ) {
                    f = scaleLengths[i] / dist
                    dist = scaleLengths[i]
                    break;
                }
            }
            if (f === 0) {
                f = dist / scaleLengths[i]
                dist = scaleLengths[i]
            }
        }
        text = formatDistance(dist)
        scaleImage.width = (scaleImage.sourceSize.width * f) - 2 * scaleImageLeft.sourceSize.width
        scaleText.text = text
    }

    zoomLevel:                  18
    center:                     QGroundControl.lastKnownHomePosition
    gesture.flickDeceleration:  3000

    plugin: Plugin { name: "QGroundControl" }

    ExclusiveGroup { id: mapTypeGroup }

    property bool _initialMapPositionSet: false

    Connections {
        target: mainWindow
        onGcsPositionChanged: {
            if (!_initialMapPositionSet) {
                _initialMapPositionSet = true
                center = mainWindow.gcsPosition
            }
        }
    }

    function updateActiveMapType() {
        var fullMapName = QGroundControl.flightMapSettings.mapProvider + " " + QGroundControl.flightMapSettings.mapType
        for (var i = 0; i < _map.supportedMapTypes.length; i++) {
            if (fullMapName === _map.supportedMapTypes[i].name) {
                _map.activeMapType = _map.supportedMapTypes[i]
                return
            }
        }
    }

    Component.onCompleted: updateActiveMapType()

    Connections {
        target:             QGroundControl.flightMapSettings
        onMapTypeChanged:   updateActiveMapType()
    }

    MapQuickItem {
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2
        visible:        mainWindow.gcsPosition.isValid
        coordinate:     mainWindow.gcsPosition
        sourceItem:     MissionItemIndexLabel {
            label: "Q"
        }
    }

    //---- Polygon drawing code

    //
    // Usage:
    //
    // Connections {
    //     target: map.polygonDraw
    //
    //    onPolygonStarted: {
    //      // Polygon creation has started
    //    }
    //
    //    onPolygonFinished: {
    //      // Polygon capture complete, coordinates signal variable contains the polygon points
    //    }
    // }
    //
    // map.polygonDraqw.startPolgyon() - begin capturing a new polygon
    // map.polygonDraqw.endPolygon() - end capture (right-click will also end capture)

    // Not sure why this is needed, but trying to reference polygonDrawer directly from other code doesn't work
    property alias polygonDraw: polygonDrawer

    QGCLabel {
        id:                     polygonHelp
        anchors.topMargin:      parent.height - ScreenTools.availableHeight
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          parent.right
        horizontalAlignment:    Text.AlignHCenter
        text:                   qsTr("Click to add point %1").arg(ScreenTools.isMobile || !polygonDrawer.polygonReady ? "" : qsTr("- Right Click to end polygon"))
        visible:                polygonDrawer.drawingPolygon
    }

    MouseArea {
        id:                 polygonDrawer
        anchors.fill:       parent
        acceptedButtons:    Qt.LeftButton | Qt.RightButton
        visible:            drawingPolygon
        z:                  1000 // Hack to fix MouseArea layering for now

        property alias  drawingPolygon: polygonDrawer.hoverEnabled
        property bool   polygonReady:   polygonDrawerPolygon.path.length > 3 ///< true: enough points have been captured to create a closed polygon

        /// New polygon capture has started
        signal polygonStarted

        /// Polygon capture is complete
        ///     @param coordinates Map coordinates for the polygon points
        signal polygonFinished(var coordinates)

        /// Begin capturing a new polygon
        ///     polygonStarted will be signalled
        function startPolygon() {
            polygonDrawer.drawingPolygon = true
            polygonDrawer._clearPolygon()
            polygonDrawer.polygonStarted()
        }

        /// Finish capturing the polygon
        ///     polygonFinished will be signalled
        /// @return true: polygon completed, false: not enough points to complete polygon
        function finishPolygon() {
            if (!polygonDrawer.polygonReady) {
                return false
            }

            var polygonPath = polygonDrawerPolygon.path
            polygonPath.pop() // get rid of drag coordinate
            polygonDrawer._clearPolygon()
            polygonDrawer.drawingPolygon = false
            polygonDrawer.polygonFinished(polygonPath)
            return true
        }

        function _clearPolygon() {
            // Simpler methods to clear the path simply don't work due to bugs. This craziness does.
            var bogusCoord = _map.toCoordinate(Qt.point(height/2, width/2))
            polygonDrawerPolygon.path = [ bogusCoord, bogusCoord ]
            polygonDrawerNextPoint.path = [ bogusCoord, bogusCoord ]
            polygonDrawerPolygon.path = [ ]
            polygonDrawerNextPoint.path = [ ]
        }

        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                if (polygonDrawerPolygon.path.length > 2) {
                    // Make sure the new line doesn't intersect the existing polygon
                    var lastSegment = polygonDrawerPolygon.path.length - 2
                    var newLineA = _map.fromCoordinate(polygonDrawerPolygon.path[lastSegment], false /* clipToViewPort */)
                    var newLineB = _map.fromCoordinate(polygonDrawerPolygon.path[lastSegment+1], false /* clipToViewPort */)
                    for (var i=0; i<lastSegment; i++) {
                        var oldLineA = _map.fromCoordinate(polygonDrawerPolygon.path[i], false /* clipToViewPort */)
                        var oldLineB = _map.fromCoordinate(polygonDrawerPolygon.path[i+1], false /* clipToViewPort */)
                        if (QGroundControl.linesIntersect(newLineA, newLineB, oldLineA, oldLineB)) {
                            return;
                        }
                    }
                }

                var clickCoordinate = _map.toCoordinate(Qt.point(mouse.x, mouse.y))
                var polygonPath = polygonDrawerPolygon.path
                if (polygonPath.length == 0) {
                    // Add first coordinate
                    polygonPath.push(clickCoordinate)
                } else {
                    // Update finalized coordinate
                    polygonPath[polygonDrawerPolygon.path.length - 1] = clickCoordinate
                }
                // Add next drag coordinate
                polygonPath.push(clickCoordinate)
                polygonDrawerPolygon.path = polygonPath
            } else if (polygonDrawer.polygonReady) {
                finishPolygon()
            }
        }

        onPositionChanged: {
            if (polygonDrawerPolygon.path.length) {
                var dragCoordinate = _map.toCoordinate(Qt.point(mouse.x, mouse.y))

                // Update drag line
                polygonDrawerNextPoint.path = [ polygonDrawerPolygon.path[polygonDrawerPolygon.path.length - 2], dragCoordinate ]

                // Update drag coordinate
                var polygonPath = polygonDrawerPolygon.path
                polygonPath[polygonDrawerPolygon.path.length - 1] = dragCoordinate
                polygonDrawerPolygon.path = polygonPath
            }
        }
    }

    MapPolygon {
        id:         polygonDrawerPolygon
        color:      "blue"
        opacity:    0.5
        visible:    polygonDrawer.drawingPolygon
    }

    MapPolyline {
        id:         polygonDrawerNextPoint
        line.color: "green"
        line.width: 5
        visible:    polygonDrawer.drawingPolygon
    }

    //---- End Polygon Drawing code
} // Map
