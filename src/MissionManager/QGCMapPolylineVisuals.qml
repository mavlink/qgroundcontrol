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

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Palette           1.0
import QGroundControl.Controls          1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ShapeFileHelper   1.0

/// QGCmapPolyline map visuals
Item {
    id: _root

    property var    qgcView                     ///< QGCView for popping dialogs
    property var    mapControl                  ///< Map control to place item in
    property var    mapPolyline                 ///< QGCMapPolyline object
    property bool   interactive:    mapPolyline.interactive
    property int    lineWidth:      3
    property color  lineColor:      "#be781c"


    property var    _polylineComponent
    property var    _dragHandlesComponent
    property var    _splitHandlesComponent

    property real _zorderDragHandle:    QGroundControl.zOrderMapItems + 3   // Highest to prevent splitting when items overlap
    property real _zorderSplitHandle:   QGroundControl.zOrderMapItems + 2

    function addVisuals() {
        _polylineComponent = polylineComponent.createObject(mapControl)
        mapControl.addMapItem(_polylineComponent)
    }

    function removeVisuals() {
        _polylineComponent.destroy()
    }

    function addHandles() {
        if (!_dragHandlesComponent) {
            _dragHandlesComponent = dragHandlesComponent.createObject(mapControl)
            _splitHandlesComponent = splitHandlesComponent.createObject(mapControl)
        }
    }

    function removeHandles() {
        if (_dragHandlesComponent) {
            _dragHandlesComponent.destroy()
            _dragHandlesComponent = undefined
        }
        if (_splitHandlesComponent) {
            _splitHandlesComponent.destroy()
            _splitHandlesComponent = undefined
        }
    }

    /// Calculate the default/initial polyline
    function defaultPolylineVertices() {
        var x = map.centerViewport.x + (map.centerViewport.width / 2)
        var yInset = map.centerViewport.height / 4
        var topPointCoord =     map.toCoordinate(Qt.point(x, map.centerViewport.y + yInset),                                false /* clipToViewPort */)
        var bottomPointCoord =  map.toCoordinate(Qt.point(x, map.centerViewport.y + map.centerViewport.height - yInset),    false /* clipToViewPort */)
        return [ topPointCoord, bottomPointCoord ]
    }

    /// Add an initial 2 point polyline
    function addInitialPolyline() {
        if (mapPolyline.count < 2) {
            mapPolyline.clear()
            var initialVertices = defaultPolylineVertices()
            mapPolyline.appendVertex(initialVertices[0])
            mapPolyline.appendVertex(initialVertices[1])
        }
    }

    /// Reset polyline back to initial default
    function resetPolyline() {
        mapPolyline.clear()
        addInitialPolyline()
    }

    onInteractiveChanged: {
        if (interactive) {
            addHandles()
        } else {
            removeHandles()
        }
    }

    Component.onCompleted: {
        addVisuals()
        if (interactive) {
            addHandles()
        }
    }

    Component.onDestruction: {
        removeVisuals()
        removeHandles()
    }

    QGCPalette { id: qgcPal }

    QGCFileDialog {
        id:             kmlLoadDialog
        qgcView:        _root.qgcView
        folder:         QGroundControl.settingsManager.appSettings.missionSavePath
        title:          qsTr("Select KML File")
        selectExisting: true
        nameFilters:    ShapeFileHelper.fileDialogKMLFilters
        fileExtension:  QGroundControl.settingsManager.appSettings.kmlFileExtension

        onAcceptedForLoad: {
            mapPolyline.loadKMLFile(file)
            close()
        }
    }

    Menu {
        id: menu
        property int _removeVertexIndex

        function popUpWithIndex(curIndex) {
            _removeVertexIndex = curIndex
            removeVertexItem.visible = mapPolyline.count > 2
            menu.popup()
        }

        MenuItem {
            id:             removeVertexItem
            text:           qsTr("Remove vertex" )
            onTriggered:    mapPolyline.removeVertex(menu._removeVertexIndex)
        }

        MenuSeparator {
            visible:        removeVertexItem.visible
        }

        MenuItem {
            text:           qsTr("Edit position..." )
            onTriggered:    qgcView.showDialog(editPositionDialog, qsTr("Edit Position"), qgcView.showDialogDefaultWidth, StandardButton.Cancel)
        }

        MenuItem {
            text:           qsTr("Load KML...")
            onTriggered:    kmlLoadDialog.openForLoad()
        }
    }

    Component {
        id: editPositionDialog

        EditPositionDialog {
            Component.onCompleted: coordinate = mapPolyline.path[menu._removeVertexIndex]
            onCoordinateChanged:  mapPolyline.adjustVertex(menu._removeVertexIndex,coordinate)
        }
    }

    Component {
        id: polylineComponent

        MapPolyline {
            line.width: lineWidth
            line.color: lineColor
            path:       mapPolyline.path
        }
    }

    Component {
        id: splitHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  splitHandle.width / 2
            anchorPoint.y:  splitHandle.height / 2

            property int vertexIndex

            sourceItem: Rectangle {
                id:             splitHandle
                width:          ScreenTools.defaultFontPixelHeight * 1.5
                height:         width
                radius:         width / 2
                border.color:   "white"
                color:          "transparent"
                opacity:        .50
                z:              _zorderSplitHandle

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.verticalCenter:     parent.verticalCenter
                    text:                       "+"
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  mapPolyline.splitSegment(mapQuickItem.vertexIndex)
                }
            }
        }
    }

    Component {
        id: splitHandlesComponent

        Repeater {
            model: mapPolyline.path

            delegate: Item {
                property var _splitHandle
                property var _vertices:     mapPolyline.path

                function _setHandlePosition() {
                    var nextIndex = index + 1
                    var distance = _vertices[index].distanceTo(_vertices[nextIndex])
                    var azimuth = _vertices[index].azimuthTo(_vertices[nextIndex])
                    _splitHandle.coordinate = _vertices[index].atDistanceAndAzimuth(distance / 2, azimuth)
                }

                Component.onCompleted: {
                    if (index + 1 <= _vertices.length - 1) {
                        _splitHandle = splitHandleComponent.createObject(mapControl)
                        _splitHandle.vertexIndex = index
                        _setHandlePosition()
                        mapControl.addMapItem(_splitHandle)
                    }
                }

                Component.onDestruction: {
                    if (_splitHandle) {
                        _splitHandle.destroy()
                    }
                }
            }
        }
    }

    // Control which is used to drag polygon vertices
    Component {
        id: dragAreaComponent

        MissionItemIndicatorDrag {
            mapControl: _root.mapControl
            id:         dragArea
            z:          _zorderDragHandle

            property int polylineVertex

            property bool _creationComplete: false

            Component.onCompleted: _creationComplete = true

            onItemCoordinateChanged: {
                if (_creationComplete) {
                    // During component creation some bad coordinate values got through which screws up draw
                    mapPolyline.adjustVertex(polylineVertex, itemCoordinate)
                }
            }

            onClicked: {
                menu.popUpWithIndex(polylineVertex)
            }

        }
    }

    Component {
        id: dragHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              _zorderDragHandle

            property int polylineVertex

            sourceItem: Rectangle {
                id:             dragHandle
                width:          ScreenTools.defaultFontPixelHeight * 1.5
                height:         width
                radius:         width * 0.5
                color:          Qt.rgba(1,1,1,0.8)
                border.color:   Qt.rgba(0,0,0,0.25)
                border.width:   1
            }
        }
    }

    // Add all polygon vertex drag handles to the map
    Component {
        id: dragHandlesComponent

        Repeater {
            model: mapPolyline.pathModel

            delegate: Item {
                property var _visuals: [ ]

                Component.onCompleted: {
                    var dragHandle = dragHandleComponent.createObject(mapControl)
                    dragHandle.coordinate = Qt.binding(function() { return object.coordinate })
                    dragHandle.polylineVertex = Qt.binding(function() { return index })
                    mapControl.addMapItem(dragHandle)
                    var dragArea = dragAreaComponent.createObject(mapControl, { "itemIndicator": dragHandle, "itemCoordinate": object.coordinate })
                    dragArea.polylineVertex = Qt.binding(function() { return index })
                    _visuals.push(dragHandle)
                    _visuals.push(dragArea)
                }

                Component.onDestruction: {
                    for (var i=0; i<_visuals.length; i++) {
                        _visuals[i].destroy()
                    }
                    _visuals = [ ]
                }
            }
        }
    }
}

