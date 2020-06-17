/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick      2.3
import QtLocation   5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0


/// Polygon drawing item. Add to your control and call methods to get support for polygon drawing and adjustment.
Item {
    id: _root

    // These properties must be provided by the consumer
    property var    map            ///< Map control
    property var    callbackObject ///< Callback item

    // These properties can be queried by the consumer
    property bool   drawingPolygon:     false
    property bool   adjustingPolygon:   false
    property bool   polygonReady:       _currentPolygon ? _currentPolygon.path.length > 2 : false   ///< true: enough points have been captured to create a closed polygon

    property var    _helpLabel                                  ///< Dynamically added help label component
    property var    _newPolygon                                 ///< Dynamically added polygon which represents all polygon points including the one currently being drawn
    property var    _currentPolygon                             ///< Dynamically added polygon which represents the currently completed polygon
    property var    _nextPointLine                              ///< Dynamically added line which goes from last polygon point to the new one being drawn
    property var    _mobileSegment                              ///< Dynamically added line between first and second polygon point for mobile
    property var    _mobilePoint                                ///< Dynamically added point showing first polygon point on mobile
    property var    _mouseArea                                  ///< Dynamically added MouseArea which handles all clicking and mouse movement
    property var    _vertexDragList:    [ ]                     ///< Dynamically added vertex drag points
    property bool   _mobile:            ScreenTools.isMobile

    /// Begin capturing a new polygon
    ///     polygonCaptureStarted will be signalled through callbackObject
    function startCapturePolygon() {
        _helpLabel =        helpLabelComponent.createObject     (map)
        _newPolygon =       newPolygonComponent.createObject    (map)
        _currentPolygon =   currentPolygonComponent.createObject(map)
        _nextPointLine =    nextPointComponent.createObject     (map)
        _mobileSegment =    mobileSegmentComponent.createObject (map)
        _mobilePoint =      mobilePointComponent.createObject   (map)
        _mouseArea =        mouseAreaComponent.createObject     (map)

        map.addMapItem(_newPolygon)
        map.addMapItem(_currentPolygon)
        map.addMapItem(_nextPointLine)
        map.addMapItem(_mobileSegment)
        map.addMapItem(_mobilePoint)

        drawingPolygon = true
        callbackObject.polygonCaptureStarted()
    }

    /// Finish capturing the polygon
    ///     polygonCaptureFinished will be signalled through callbackObject
    /// @return true: polygon completed, false: not enough points to complete polygon
    function finishCapturePolygon() {
        if (!polygonReady) {
            return false
        }
        var polygonPath = _currentPolygon.path
        _cancelCapturePolygon()
        callbackObject.polygonCaptureFinished(polygonPath)
        return true
    }

    function startAdjustPolygon(vertexCoordinates) {
        adjustingPolygon = true
        for (var i=0; i<vertexCoordinates.length; i++) {
            var dragItem = Qt.createQmlObject(
                        "import QtQuick                     2.3; " +
                        "import QtLocation                  5.3; " +
                        "import QGroundControl.ScreenTools  1.0; " +
                        "" +
                        "Rectangle {" +
                        "   id:     vertexDrag; " +
                        "   width:  _sideLength + _expandMargin; " +
                        "   height: _sideLength + _expandMargin; " +
                        "   color:  'red'; " +
                        "" +
                        "   property var coordinate; " +
                        "   property int index; " +
                        "" +
                        "   readonly property real _sideLength:     ScreenTools.defaultFontPixelWidth * 2; " +
                        "   readonly property real _halfSideLength: _sideLength / 2; " +
                        "" +
                        "   property real _expandMargin: ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth : 0;" +
                        "" +
                        "   Drag.active:    dragMouseArea.drag.active; " +
                        "" +
                        "   onXChanged: updateCoordinate(); " +
                        "   onYChanged: updateCoordinate(); " +
                        "" +
                        "   function updateCoordinate() { " +
                        "       vertexDrag.coordinate = map.toCoordinate(Qt.point(vertexDrag.x + _expandMargin + _halfSideLength, vertexDrag.y + _expandMargin + _halfSideLength), false); " +
                        "       callbackObject.polygonAdjustVertex(vertexDrag.index, vertexDrag.coordinate); " +
                        "   } " +
                        "" +
                        "   function updatePosition() { " +
                        "       var vertexPoint = map.fromCoordinate(coordinate, false); " +
                        "       vertexDrag.x = vertexPoint.x - _expandMargin - _halfSideLength; " +
                        "       vertexDrag.y = vertexPoint.y - _expandMargin - _halfSideLength; " +
                        "   } " +
                        "" +
                        "   Connections { " +
                        "       target:             map; " +
                        "       onCenterChanged:    updatePosition(); " +
                        "       onZoomLevelChanged: updatePosition(); " +
                        "   } " +
                        "" +
                        "   MouseArea { " +
                        "       id:             dragMouseArea; " +
                        "       anchors.fill:   parent; " +
                        "       drag.target:    parent; " +
                        "       drag.minimumX:  0; " +
                        "       drag.minimumY:  0; " +
                        "       drag.maximumX:  map.width - parent.width; " +
                        "       drag.maximumY:  map.height - parent.height; " +
                        "   } " +
                        "} ",
                        map)
            dragItem.z = QGroundControl.zOrderMapItems + 1
            dragItem.coordinate = vertexCoordinates[i]
            dragItem.index = i
            dragItem.updatePosition()
            _vertexDragList.push(dragItem)
            callbackObject.polygonAdjustStarted()
        }
    }

    function finishAdjustPolygon() {
        _cancelAdjustPolygon()
        callbackObject.polygonAdjustFinished()
    }

    /// Cancels an in progress draw or adjust
    function cancelPolygonEdit() {
        _cancelAdjustPolygon()
        _cancelCapturePolygon()
    }

    function _cancelAdjustPolygon() {
        adjustingPolygon = false
        for (var i=0; i<_vertexDragList.length; i++) {
            _vertexDragList[i].destroy()
        }
        _vertexDragList = []
    }

    function _cancelCapturePolygon() {
        _helpLabel.destroy()
        _newPolygon.destroy()
        _currentPolygon.destroy()
        _nextPointLine.destroy()
        _mouseArea.destroy()
        drawingPolygon = false
    }

    Component {
        id: helpLabelComponent

        QGCMapLabel {
            id:                     polygonHelp
            anchors.topMargin:      parent.height - mainWindow.height
            anchors.top:            parent.top
            anchors.left:           parent.left
            anchors.right:          parent.right
            horizontalAlignment:    Text.AlignHCenter
            map:                    _root.map
            text:                   qsTr("Click to add point %1").arg(ScreenTools.isMobile || !polygonReady ? "" : qsTr("- Right Click to end polygon"))

            Connections {
                target: _root

                onDrawingPolygonChanged: {
                    if (drawingPolygon) {
                        polygonHelp.text = qsTr("Click to add point")
                    }
                    polygonHelp.visible = drawingPolygon
                }

                onPolygonReadyChanged: {
                    if (polygonReady && !ScreenTools.isMobile) {
                        polygonHelp.text = qsTr("Click to add point - Right Click to end polygon")
                    }
                }

                onAdjustingPolygonChanged: {
                    if (adjustingPolygon) {
                        polygonHelp.text = qsTr("Adjust polygon by dragging corners")
                    }
                    polygonHelp.visible = adjustingPolygon
                }
            }
        }
    }

    Component {
        id: mouseAreaComponent

        MouseArea {
            anchors.fill:       map
            acceptedButtons:    Qt.LeftButton | Qt.RightButton
            hoverEnabled:       true
            z:                  QGroundControl.zOrderMapItems + 1

            property bool   justClicked: false

            onClicked: {
                if (mouse.button == Qt.LeftButton) {
                    justClicked = true
                    if (_newPolygon.path.length > 2) {
                        // Make sure the new line doesn't intersect the existing polygon
                        var lastSegment = _newPolygon.path.length - 2
                        var newLineA = map.fromCoordinate(_newPolygon.path[lastSegment], false /* clipToViewPort */)
                        var newLineB = map.fromCoordinate(_newPolygon.path[lastSegment+1], false /* clipToViewPort */)
                        for (var i=0; i<lastSegment; i++) {
                            var oldLineA = map.fromCoordinate(_newPolygon.path[i], false /* clipToViewPort */)
                            var oldLineB = map.fromCoordinate(_newPolygon.path[i+1], false /* clipToViewPort */)
                            if (QGroundControl.linesIntersect(newLineA, newLineB, oldLineA, oldLineB)) {
                                return;
                            }
                        }
                    }

                    var clickCoordinate = map.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                    var polygonPath = _newPolygon.path
                    if (polygonPath.length === 0) {
                        // Add first coordinate
                        polygonPath.push(clickCoordinate)
                    } else {
                        // Add subsequent coordinate
                        if (ScreenTools.isMobile) {
                            // Since mobile has no mouse, the onPositionChangedHandler will not fire. We have to add the coordinate
                            // here instead.
                            justClicked = false
                            polygonPath.push(clickCoordinate)
                        } else {
                            // The onPositionChanged handler for mouse movement will have already added the coordinate to the array.
                            // Just update it to the final position
                            polygonPath[_newPolygon.path.length - 1] = clickCoordinate
                        }
                    }
                    _currentPolygon.path = polygonPath
                    _newPolygon.path = polygonPath

                    if (_mobile && _currentPolygon.path.length === 1) {
                        _mobilePoint.coordinate = _currentPolygon.path[0]
                        _mobilePoint.visible = true
                    } else if (_mobile && _currentPolygon.path.length === 2) {
                        // Show initial line segment on mobile
                        _mobileSegment.path = [ _currentPolygon.path[0], _currentPolygon.path[1] ]
                        _mobileSegment.visible = true
                        _mobilePoint.visible = false
                    } else {
                        _mobileSegment.visible = false
                        _mobilePoint.visible = false
                    }
                } else if (polygonReady) {
                    finishCapturePolygon()
                }
            }

            onPositionChanged: {
                if (ScreenTools.isMobile) {
                    // We don't track mouse drag on mobile
                    return
                }
                if (_newPolygon.path.length) {
                    var dragCoordinate = map.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                    var polygonPath = _newPolygon.path
                    if (justClicked){
                        // Add new drag coordinate
                        polygonPath.push(dragCoordinate)
                        justClicked = false
                    }

                    // Update drag line
                    _nextPointLine.path = [ _newPolygon.path[_newPolygon.path.length - 2], dragCoordinate ]

                    polygonPath[_newPolygon.path.length - 1] = dragCoordinate
                    _newPolygon.path = polygonPath
                }
            }
        }
    }

    /// Polygon being drawn, including new point
    Component {
        id: newPolygonComponent

        MapPolygon {
            color:      "blue"
            opacity:    0.5
            visible:    path.length > 2
        }
    }

    /// Current complete polygon
    Component {
        id: currentPolygonComponent

        MapPolygon {
            color:      'green'
            opacity:    0.5
            visible:    polygonReady
        }
    }

    /// First line segment to show on mobile
    Component {
        id: mobileSegmentComponent

        MapPolyline {
            line.color: "green"
            line.width: 3
            visible:    false
        }
    }

    /// First line segment to show on mobile
    Component {
        id: mobilePointComponent

        MapQuickItem {
            anchorPoint.x:  rect.width / 2
            anchorPoint.y:  rect.height / 2
            visible:        false

            sourceItem: Rectangle {
                id:     rect
                width:  ScreenTools.defaultFontPixelHeight
                height: width
                color:  "green"
            }
        }
    }

    /// Next line for polygon
    Component {
        id: nextPointComponent

        MapPolyline {
            line.color: "green"
            line.width: 3
        }
    }
}
