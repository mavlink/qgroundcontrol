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

/// QGCMapPolygon map visuals
Item {
    id: _root

    property var    qgcView                                     ///< QGCView for popping dialogs
    property var    mapControl                                  ///< Map control to place item in
    property var    mapPolygon                                  ///< QGCMapPolygon object
    property bool   interactive:        mapPolygon.interactive
    property color  interiorColor:      "transparent"
    property real   interiorOpacity:    1
    property int    borderWidth:        0
    property color  borderColor:        "black"

    property var    _polygonComponent
    property var    _dragHandlesComponent
    property var    _splitHandlesComponent
    property var    _centerDragHandleComponent
    property bool   _circle:                    false
    property real   _circleRadius
    property bool   _editCircleRadius:          false

    property real _zorderDragHandle:    QGroundControl.zOrderMapItems + 3   // Highest to prevent splitting when items overlap
    property real _zorderSplitHandle:   QGroundControl.zOrderMapItems + 2
    property real _zorderCenterHandle:  QGroundControl.zOrderMapItems + 1   // Lowest such that drag or split takes precedence

    function addVisuals() {
        _polygonComponent = polygonComponent.createObject(mapControl)
        mapControl.addMapItem(_polygonComponent)
    }

    function removeVisuals() {
        _polygonComponent.destroy()
    }

    function addHandles() {
        if (!_dragHandlesComponent) {
            _dragHandlesComponent = dragHandlesComponent.createObject(mapControl)
            _splitHandlesComponent = splitHandlesComponent.createObject(mapControl)
            _centerDragHandleComponent = centerDragHandleComponent.createObject(mapControl)
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
        if (_centerDragHandleComponent) {
            _centerDragHandleComponent.destroy()
            _centerDragHandleComponent = undefined
        }
    }

    /// Calculate the default/initial 4 sided polygon
    function defaultPolygonVertices() {
        // Initial polygon is inset to take 2/3rds space
        var rect = Qt.rect(map.centerViewport.x, map.centerViewport.y, map.centerViewport.width, map.centerViewport.height)
        rect.x += (rect.width * 0.25) / 2
        rect.y += (rect.height * 0.25) / 2
        rect.width *= 0.75
        rect.height *= 0.75

        var centerCoord =       map.toCoordinate(Qt.point(rect.x + (rect.width / 2), rect.y + (rect.height / 2)),   false /* clipToViewPort */)
        var topLeftCoord =      map.toCoordinate(Qt.point(rect.x, rect.y),                                          false /* clipToViewPort */)
        var topRightCoord =     map.toCoordinate(Qt.point(rect.x + rect.width, rect.y),                             false /* clipToViewPort */)
        var bottomLeftCoord =   map.toCoordinate(Qt.point(rect.x, rect.y + rect.height),                            false /* clipToViewPort */)
        var bottomRightCoord =  map.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height),               false /* clipToViewPort */)

        // Initial polygon has max width and height of 3000 meters
        var halfWidthMeters =   Math.min(topLeftCoord.distanceTo(topRightCoord), 3000) / 2
        var halfHeightMeters =  Math.min(topLeftCoord.distanceTo(bottomLeftCoord), 3000) / 2
        topLeftCoord =      centerCoord.atDistanceAndAzimuth(halfWidthMeters, -90).atDistanceAndAzimuth(halfHeightMeters, 0)
        topRightCoord =     centerCoord.atDistanceAndAzimuth(halfWidthMeters, 90).atDistanceAndAzimuth(halfHeightMeters, 0)
        bottomLeftCoord =   centerCoord.atDistanceAndAzimuth(halfWidthMeters, -90).atDistanceAndAzimuth(halfHeightMeters, 180)
        bottomRightCoord =  centerCoord.atDistanceAndAzimuth(halfWidthMeters, 90).atDistanceAndAzimuth(halfHeightMeters, 180)

        return [ topLeftCoord, topRightCoord, bottomRightCoord, bottomLeftCoord, centerCoord  ]
    }

    /// Add an initial 4 sided polygon
    function addInitialPolygon() {
        if (mapPolygon.count < 3) {
            initialVertices = defaultPolygonVertices()
            mapPolygon.appendVertex(initialVertices[0])
            mapPolygon.appendVertex(initialVertices[1])
            mapPolygon.appendVertex(initialVertices[2])
            mapPolygon.appendVertex(initialVertices[3])
        }
    }

    /// Reset polygon back to initial default
    function resetPolygon() {
        var initialVertices = defaultPolygonVertices()
        mapPolygon.clear()
        for (var i=0; i<4; i++) {
            mapPolygon.appendVertex(initialVertices[i])
        }
        _circle = false
    }

    function setCircleRadius(center, radius) {
        var unboundCenter = center.atDistanceAndAzimuth(0, 0)
        _circleRadius = radius
        var segments = 16
        var angleIncrement = 360 / segments
        var angle = 0
        mapPolygon.clear()
        for (var i=0; i<segments; i++) {
            var vertex = unboundCenter.atDistanceAndAzimuth(_circleRadius, angle)
            mapPolygon.appendVertex(vertex)
            angle += angleIncrement
        }
        _circle = true
    }

    /// Reset polygon to a circle which fits within initial polygon
    function resetCircle() {
        var initialVertices = defaultPolygonVertices()
        var width = initialVertices[0].distanceTo(initialVertices[1])
        var height = initialVertices[1].distanceTo(initialVertices[2])
        var radius = Math.min(width, height) / 2
        var center = initialVertices[4]
        setCircleRadius(center, radius)
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
        id:             kmlOrSHPLoadDialog
        qgcView:        _root.qgcView
        folder:         QGroundControl.settingsManager.appSettings.missionSavePath
        title:          qsTr("Select Polygon File")
        selectExisting: true
        nameFilters:    ShapeFileHelper.fileDialogKMLOrSHPFilters
        fileExtension:  QGroundControl.settingsManager.appSettings.kmlFileExtension
        fileExtension2: QGroundControl.settingsManager.appSettings.shpFileExtension

        onAcceptedForLoad: {
            mapPolygon.loadKMLOrSHPFile(file)
            mapFitFunctions.fitMapViewportToMissionItems()
            close()
        }
    }

    Menu {
        id: menu

        property int _editingVertexIndex: -1

        function popupVertex(curIndex) {
            menu._editingVertexIndex = curIndex
            removeVertexItem.visible = (mapPolygon.count > 3 && menu._editingVertexIndex >= 0)
            menu.popup()
        }

        function popupCenter() {
            menu.popup()
        }

        MenuItem {
            id:             removeVertexItem
            visible:        !_circle
            text:           qsTr("Remove vertex")
            onTriggered: {
                if (menu._editingVertexIndex >= 0) {
                    mapPolygon.removeVertex(menu._editingVertexIndex)
                }
            }
        }

        MenuSeparator {
            visible:        removeVertexItem.visible
        }

        MenuItem {
            text:           qsTr("Circle" )
            onTriggered:    resetCircle()
        }

        MenuItem {
            text:           qsTr("Polygon")
            onTriggered:    resetPolygon()
        }

        MenuItem {
            text:           qsTr("Set radius..." )
            visible:        _circle
            onTriggered:    _editCircleRadius = true
        }

        MenuItem {
            text:           qsTr("Edit position..." )
            visible:        _circle
            onTriggered:    qgcView.showDialog(editCenterPositionDialog, qsTr("Edit Center Position"), qgcView.showDialogDefaultWidth, StandardButton.Close)
        }

        MenuItem {
            text:           qsTr("Edit position..." )
            visible:        !_circle && menu._editingVertexIndex >= 0
            onTriggered:    qgcView.showDialog(editVertexPositionDialog, qsTr("Edit Vertex Position"), qgcView.showDialogDefaultWidth, StandardButton.Close)
        }

        MenuItem {
            text:           qsTr("Load KML/SHP...")
            onTriggered:    kmlOrSHPLoadDialog.openForLoad()
        }
    }

    Component {
        id: polygonComponent

        MapPolygon {
            color:          interiorColor
            opacity:        interiorOpacity
            border.color:   borderColor
            border.width:   borderWidth
            path:           mapPolygon.path
        }
    }

    Component {
        id: splitHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            visible:        !_circle

            property int vertexIndex

            sourceItem: Rectangle {
                id:         dragHandle
                width:      ScreenTools.defaultFontPixelHeight * 1.5
                height:     width
                radius:     width / 2
                border.color:      "white"
                color:      "transparent"
                opacity:    .50
                z:          _zorderSplitHandle

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.verticalCenter:     parent.verticalCenter
                    text:                       "+"
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  mapPolygon.splitPolygonSegment(mapQuickItem.vertexIndex)
                }
            }
        }
    }

    Component {
        id: splitHandlesComponent

        Repeater {
            model: mapPolygon.path

            delegate: Item {
                property var _splitHandle
                property var _vertices:     mapPolygon.path

                function _setHandlePosition() {
                    var nextIndex = index + 1
                    if (nextIndex > _vertices.length - 1) {
                        nextIndex = 0
                    }
                    var distance = _vertices[index].distanceTo(_vertices[nextIndex])
                    var azimuth = _vertices[index].azimuthTo(_vertices[nextIndex])
                    _splitHandle.coordinate = _vertices[index].atDistanceAndAzimuth(distance / 2, azimuth)
                }

                Component.onCompleted: {
                    _splitHandle = splitHandleComponent.createObject(mapControl)
                    _splitHandle.vertexIndex = index
                    _setHandlePosition()
                    mapControl.addMapItem(_splitHandle)
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
            id:         dragArea
            mapControl: _root.mapControl
            z:          _zorderDragHandle
            visible:    !_circle
            onDragStop: mapPolygon.verifyClockwiseWinding()

            property int polygonVertex

            property bool _creationComplete: false

            Component.onCompleted: _creationComplete = true

            onItemCoordinateChanged: {
                if (_creationComplete) {
                    // During component creation some bad coordinate values got through which screws up draw
                    mapPolygon.adjustVertex(polygonVertex, itemCoordinate)
                }
            }

            onClicked: menu.popupVertex(polygonVertex)
        }
    }

    Component {
        id: centerDragHandle
        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width  * 0.5
            anchorPoint.y:  dragHandle.height * 0.5
            z:              _zorderDragHandle
            sourceItem: Rectangle {
                id:             dragHandle
                width:          ScreenTools.defaultFontPixelHeight * 1.5
                height:         width
                radius:         width * 0.5
                color:          Qt.rgba(1,1,1,0.8)
                border.color:   Qt.rgba(0,0,0,0.25)
                border.width:   1
                QGCColoredImage {
                    width:      parent.width
                    height:     width
                    color:      Qt.rgba(0,0,0,1)
                    mipmap:     true
                    fillMode:   Image.PreserveAspectFit
                    source:     "/qmlimages/MapCenter.svg"
                    sourceSize.height:  height
                    anchors.centerIn:   parent
                }
            }
        }
    }

    Component {
        id: dragHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width  / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              _zorderDragHandle
            visible:        !_circle

            property int polygonVertex

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
            model: mapPolygon.pathModel

            delegate: Item {
                property var _visuals: [ ]

                Component.onCompleted: {
                    var dragHandle = dragHandleComponent.createObject(mapControl)
                    dragHandle.coordinate = Qt.binding(function() { return object.coordinate })
                    dragHandle.polygonVertex = Qt.binding(function() { return index })
                    mapControl.addMapItem(dragHandle)
                    var dragArea = dragAreaComponent.createObject(mapControl, { "itemIndicator": dragHandle, "itemCoordinate": object.coordinate })
                    dragArea.polygonVertex = Qt.binding(function() { return index })
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

    Component {
        id: editCenterPositionDialog

        EditPositionDialog {
            coordinate: mapPolygon.center
            onCoordinateChanged: {
                // Prevent spamming signals on vertex changes by setting centerDrag = true when changing center position.
                // This also fixes a bug where Qt gets confused by all the signalling and draws a bad visual.
                mapPolygon.centerDrag = true
                mapPolygon.center = coordinate
                mapPolygon.centerDrag = false
            }
        }
    }

    Component {
        id: editVertexPositionDialog

        EditPositionDialog {
            coordinate:             mapPolygon.vertexCoordinate(menu._editingVertexIndex)
            onCoordinateChanged: {
                mapPolygon.adjustVertex(menu._editingVertexIndex, coordinate)
                mapPolygon.verifyClockwiseWinding()
            }
        }
    }

    Component {
        id: centerDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl:                 _root.mapControl
            z:                          _zorderCenterHandle
            onItemCoordinateChanged:    mapPolygon.center = itemCoordinate
            onDragStart:                mapPolygon.centerDrag = true
            onDragStop:                 mapPolygon.centerDrag = false

            onClicked: menu.popupCenter()

            function setRadiusFromDialog() {
                var radius = QGroundControl.appSettingsDistanceUnitsToMeters(radiusField.text)
                setCircleRadius(mapPolygon.center, radius)
                _editCircleRadius = false
            }

            Rectangle {
                anchors.margins:    _margin
                anchors.left:       parent.right
                width:              radiusColumn.width + (_margin *2)
                height:             radiusColumn.height + (_margin *2)
                color:              qgcPal.window
                border.color:       qgcPal.text
                visible:            _editCircleRadius

                Column {
                    id:                 radiusColumn
                    anchors.margins:    _margin
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            _margin

                    QGCLabel { text: qsTr("Radius:") }

                    QGCTextField {
                        id:                 radiusField
                        showUnits:          true
                        unitsLabel:         QGroundControl.appSettingsDistanceUnitsString
                        text:               QGroundControl.metersToAppSettingsDistanceUnits(_circleRadius).toFixed(2)
                        onEditingFinished:  setRadiusFromDialog()
                        inputMethodHints:   Qt.ImhFormattedNumbersOnly
                    }
                }

                QGCLabel {
                    anchors.right:  radiusColumn.right
                    anchors.top:    radiusColumn.top
                    text:           "X"

                    QGCMouseArea {
                        fillItem:   parent
                        onClicked:  setRadiusFromDialog()
                    }
                }
            }
        }
    }

    Component {
        id: centerDragHandleComponent

        Item {
            property var dragHandle
            property var dragArea

            Component.onCompleted: {
                dragHandle = centerDragHandle.createObject(mapControl)
                dragHandle.coordinate = Qt.binding(function() { return mapPolygon.center })
                mapControl.addMapItem(dragHandle)
                dragArea = centerDragAreaComponent.createObject(mapControl, { "itemIndicator": dragHandle, "itemCoordinate": mapPolygon.center })
            }

            Component.onDestruction: {
                dragHandle.destroy()
                dragArea.destroy()
            }
        }
    }
}

