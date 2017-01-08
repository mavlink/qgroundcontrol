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

    function setVisibleRegion(region) {
        // This works around a bug on Qt where if you set a visibleRegion and then the user moves or zooms the map
        // and then you set the same visibleRegion the map will not move/scale appropriately since it thinks there
        // is nothing to do.
        _map.visibleRegion = QtPositioning.rectangle(QtPositioning.coordinate(0, 0), QtPositioning.coordinate(0, 0))
        _map.visibleRegion = region
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

    /// Ground Station location
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
    //    onPolygonCaptureStarted: {
    //      // Polygon creation has started
    //    }
    //
    //    onPolygonCaptureFinished: {
    //      // Polygon capture complete, coordinates signal variable contains the polygon points
    //    }
    // }
    //
    // map.polygonDraqw.startPolgyon() - begin capturing a new polygon
    // map.polygonDraqw.endPolygon() - end capture (right-click will also end capture)

    // Not sure why this is needed, but trying to reference polygonDrawer directly from other code doesn't work
    property alias polygonDraw: polygonDrawer

    QGCMapLabel {
        id:                     polygonHelp        
        anchors.topMargin:      parent.height - ScreenTools.availableHeight
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          parent.right
        horizontalAlignment:    Text.AlignHCenter
        map:                    _map
        text:                   qsTr("Click to add point %1").arg(ScreenTools.isMobile || !polygonDrawer.polygonReady ? "" : qsTr("- Right Click to end polygon"))
        visible:                polygonDrawer.drawingPolygon

        Connections {
            target: polygonDrawer

            onDrawingPolygonChanged: {
                if (polygonDrawer.drawingPolygon) {
                    polygonHelp.text = qsTr("Click to add point")
                }
                polygonHelp.visible = polygonDrawer.drawingPolygon
            }

            onPolygonReadyChanged: {
                if (polygonDrawer.polygonReady && !ScreenTools.isMobile) {
                    polygonHelp.text = qsTr("Click to add point - Right Click to end polygon")
                }
            }

            onAdjustingPolygonChanged: {
                if (polygonDrawer.adjustingPolygon) {
                    polygonHelp.text = qsTr("Adjust polygon by dragging corners")
                }
                polygonHelp.visible = polygonDrawer.adjustingPolygon
            }
        }
    }

    MouseArea {
        id:                 polygonDrawer
        anchors.fill:       parent
        acceptedButtons:    Qt.LeftButton | Qt.RightButton
        visible:            drawingPolygon
        z:                  1000 // Hack to fix MouseArea layering for now

        property alias  drawingPolygon:     polygonDrawer.hoverEnabled
        property bool   adjustingPolygon:   false
        property bool   polygonReady:       polygonDrawerPolygonSet.path.length > 2 ///< true: enough points have been captured to create a closed polygon
        property bool   justClicked: false

        property var _callbackObject

        property var _vertexDragList: []

        /// Begin capturing a new polygon
        ///     polygonCaptureStarted will be signalled
        function startCapturePolygon(callback) {
            polygonDrawer._callbackObject = callback
            polygonDrawer.drawingPolygon = true
            polygonDrawer._clearPolygon()
            polygonDrawer._callbackObject.polygonCaptureStarted()
        }

        /// Finish capturing the polygon
        ///     polygonCaptureFinished will be signalled
        /// @return true: polygon completed, false: not enough points to complete polygon
        function finishCapturePolygon() {
            if (!polygonDrawer.polygonReady) {
                return false
            }

            var polygonPath = polygonDrawerPolygonSet.path
            _cancelCapturePolygon()
            polygonDrawer._callbackObject.polygonCaptureFinished(polygonPath)
            return true
        }

        function startAdjustPolygon(callback, vertexCoordinates) {
            polygonDraw._callbackObject = callback
            polygonDrawer.adjustingPolygon = true
            for (var i=0; i<vertexCoordinates.length; i++) {
                var mapItem = Qt.createQmlObject(
                            "import QtQuick                     2.5; " +
                            "import QtLocation                  5.3; " +
                            "import QGroundControl.ScreenTools  1.0; " +
                            "Rectangle {" +
                            "   id:     vertexDrag; " +
                            "   width:  _sideLength; " +
                            "   height: _sideLength; " +
                            "   color:  'red'; " +
                            "" +
                            "   property var coordinate; " +
                            "   property int index; " +
                            "" +
                            "   readonly property real _sideLength:     ScreenTools.defaultFontPixelWidth * 2; " +
                            "   readonly property real _halfSideLength: _sideLength / 2; " +
                            "" +
                            "   Drag.active:    dragMouseArea.drag.active; " +
                            "   Drag.hotSpot.x: _halfSideLength; " +
                            "   Drag.hotSpot.y: _halfSideLength; " +
                            "" +
                            "   onXChanged: updateCoordinate(); " +
                            "   onYChanged: updateCoordinate(); " +
                            "" +
                            "   function updateCoordinate() { " +
                            "       vertexDrag.coordinate = _map.toCoordinate(Qt.point(vertexDrag.x + _halfSideLength, vertexDrag.y + _halfSideLength), false); " +
                            "       polygonDrawer._callbackObject.polygonAdjustVertex(vertexDrag.index, vertexDrag.coordinate); " +
                            "   } " +
                            "" +
                            "   function updatePosition() { " +
                            "       var vertexPoint = _map.fromCoordinate(coordinate, false); " +
                            "       vertexDrag.x = vertexPoint.x - _halfSideLength; " +
                            "       vertexDrag.y = vertexPoint.y - _halfSideLength; " +
                            "   } " +
                            "" +
                            "   Connections { " +
                            "       target: _map; " +
                            "       onCenterChanged: updatePosition(); " +
                            "       onZoomLevelChanged: updatePosition(); " +
                            "   } " +
                            "" +
                            "   MouseArea { " +
                            "       id:             dragMouseArea; " +
                            "       anchors.fill:   parent; " +
                            "       drag.target:    parent; " +
                            "       drag.minimumX:  0; " +
                            "       drag.minimumY:  0; " +
                            "       drag.maximumX:  _map.width - parent.width; " +
                            "       drag.maximumY:  _map.height - parent.height; " +
                            "   } " +
                            "} ",
                            _map)
                mapItem.z = QGroundControl.zOrderMapItems + 1
                mapItem.coordinate = vertexCoordinates[i]
                mapItem.index = i
                mapItem.updatePosition()
                polygonDrawer._vertexDragList.push(mapItem)
                polygonDrawer._callbackObject.polygonAdjustStarted()
            }
        }

        function finishAdjustPolygon() {
            _cancelAdjustPolygon()
            polygonDrawer._callbackObject.polygonAdjustFinished()
        }

        /// Cancels an in progress draw or adjust
        function cancelPolygonEdit() {
            _cancelAdjustPolygon()
            _cancelCapturePolygon()
        }

        function _cancelAdjustPolygon() {
            polygonDrawer.adjustingPolygon = false
            for (var i=0; i<polygonDrawer._vertexDragList.length; i++) {
                polygonDrawer._vertexDragList[i].destroy()
            }
            polygonDrawer._vertexDragList = []
        }

        function _cancelCapturePolygon() {
            polygonDrawer._clearPolygon()
            polygonDrawer.drawingPolygon = false
        }

        function _clearPolygon() {
            // Simpler methods to clear the path simply don't work due to bugs. This craziness does.
            var bogusCoord = _map.toCoordinate(Qt.point(height/2, width/2))
            polygonDrawerPolygon.path = [ bogusCoord, bogusCoord ]
            polygonDrawerNextPoint.path = [ bogusCoord, bogusCoord ]
            polygonDrawerPolygon.path = [ ]
            polygonDrawerNextPoint.path = [ ]
            polygonDrawerPolygonSet.path = [ bogusCoord, bogusCoord ]
            polygonDrawerPolygonSet.path = [ ]
        }

        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                polygonDrawer.justClicked = true
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
                    // Add subsequent coordinate
                    if (ScreenTools.isMobile) {
                        // Since mobile has no mouse, the onPositionChangedHandler will not fire. We have to add the coordinate
                        // here instead.
                        polygonDrawer.justClicked = false
                        polygonPath.push(clickCoordinate)
                    } else {
                        // The onPositionChanged handler for mouse movement will have already added the coordinate to the array.
                        // Just update it to the final position
                        polygonPath[polygonDrawerPolygon.path.length - 1] = clickCoordinate
                    }
                }
                polygonDrawerPolygonSet.path = polygonPath
                polygonDrawerPolygon.path = polygonPath
            } else if (polygonDrawer.polygonReady) {
                finishCapturePolygon()
            }
        }

        onPositionChanged: {
            if (ScreenTools.isMobile) {
                // We don't track mouse drag on mobile
                return
            }
            if (polygonDrawerPolygon.path.length) {
                var dragCoordinate = _map.toCoordinate(Qt.point(mouse.x, mouse.y))
                var polygonPath = polygonDrawerPolygon.path
                if (polygonDrawer.justClicked){
                    // Add new drag coordinate
                    polygonPath.push(dragCoordinate)
                    polygonDrawer.justClicked = false
                }

                // Update drag line
                polygonDrawerNextPoint.path = [ polygonDrawerPolygon.path[polygonDrawerPolygon.path.length - 2], dragCoordinate ]

                polygonPath[polygonDrawerPolygon.path.length - 1] = dragCoordinate
                polygonDrawerPolygon.path = polygonPath

            }
        }
    }

    /// Polygon being drawn
    MapPolygon {
        id:         polygonDrawerPolygon
        color:      "blue"
        opacity:    0.5
        visible:    polygonDrawerPolygon.path.length > 2
    }
    MapPolygon {
        id:         polygonDrawerPolygonSet
        color:      'green'
        opacity:    0.5
        visible:    polygonDrawer.polygonReady
    }

    /// Next line for polygon
    MapPolyline {
        id:         polygonDrawerNextPoint
        line.color: "green"
        line.width: 3
        visible:    polygonDrawer.drawingPolygon
    }

    //---- End Polygon Drawing code
} // Map
