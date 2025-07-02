/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.ShapeFileHelper

Item {
    id: _root
    property var             mapControl
    property var             mapPolygon
    property var             missionItems
    property bool            interactive         : mapPolygon.interactive
    property color           interiorColor       : "transparent"
    property color           altColor            : "transparent"
    property real            interiorOpacity     : 1
    property int             borderWidth         : 0
    property color           borderColor         : "black"
    property string          _instructionText    : _polygonToolsText
    property var             _StoreCoordinates  : []
    property real            _zorderDragHandle   : QGroundControl.zOrderMapItems + 3
    property real            _zorderSplitHandle  : QGroundControl.zOrderMapItems + 2
    property real            _zorderCenterHandle : QGroundControl.zOrderMapItems + 1
    readonly property string _polygonToolsText   : qsTr("Option")
    readonly property string _fenceText          : qsTr("Click on the map to add vertices. Click 'Done Fencing' when finished.")
    property string          someParameter       : "defaultParameter"
    property real            radius              : ScreenTools.defaultFontPixelHeight * 4.44     //Automatic Geofence Radius
    property real            centerX             //Initial Drone X-Position
    property real            centerY             //Initial Drone Y-Position
    property bool            resetChecked

    //Create Geofence Polygon Visuals
    function addCommonVisuals() {
        if (_objMgrCommonVisuals.empty) {
            _objMgrCommonVisuals.createObject(polygonComponent, mapControl, true)
        }
    }

    //Delete Geofence Polygon Visuals
    function removeCommonVisuals() {
        _objMgrCommonVisuals.destroyObjects()
    }

    //Create Editing tools for Polygon Visuals
    function addEditingVisuals() {
        if (_objMgrEditingVisuals.empty) {
            _objMgrEditingVisuals.createObjects([ dragHandlesComponent, splitHandlesComponent, centerDragHandleComponent ], mapControl, false /* addToMap */)
        }
    }

    //Delete Editing tools for Polygon Visuals
    function removeEditingVisuals() {
        _objMgrEditingVisuals.destroyObjects()
    }

    // Create a Toolbar for Polygon Visuals
    function addToolbarVisuals() {
        if (_objMgrToolVisuals.empty) {
            var toolbar = _objMgrToolVisuals.createObject(toolbarComponent, mapControl)
            toolbar.z = QGroundControl.zOrderWidgets
        }
    }

    // Delete a Toolbar for Polygon Visuals
    function removeToolVisuals() {
        _objMgrToolVisuals.destroyObjects()
    }

    /// Calculate the default/initial 4 sided polygon
    function defaultPolygonVertices() {
        // Initial polygon is inset to take 2/3rds space
        var rect = Qt.rect(mapControl.centerViewport.x, mapControl.centerViewport.y, mapControl.centerViewport.width, mapControl.centerViewport.height)
        rect.x += (rect.width * 0.25) / 2
        rect.y += (rect.height * 0.25) / 2
        rect.width *= 0.75
        rect.height *= 0.75

        var centerCoord      =  mapControl.toCoordinate(Qt.point(rect.x + (rect.width / 2), rect.y + (rect.height / 2)),   false /* clipToViewPort */)
        var topLeftCoord     =  mapControl.toCoordinate(Qt.point(rect.x, rect.y),                                          false /* clipToViewPort */)
        var topRightCoord    =  mapControl.toCoordinate(Qt.point(rect.x + rect.width, rect.y),                             false /* clipToViewPort */)
        var bottomLeftCoord  =  mapControl.toCoordinate(Qt.point(rect.x, rect.y + rect.height),                            false /* clipToViewPort */)
        var bottomRightCoord =  mapControl.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height),               false /* clipToViewPort */)

        // Initial polygon has max width and height of 3000 meters
        var halfWidthMeters  =  Math.min(topLeftCoord.distanceTo(topRightCoord), 3000) / 2
        var halfHeightMeters =  Math.min(topLeftCoord.distanceTo(bottomLeftCoord), 3000) / 2
        topLeftCoord         =  centerCoord.atDistanceAndAzimuth(halfWidthMeters, -90).atDistanceAndAzimuth(halfHeightMeters, 0)
        topRightCoord        =  centerCoord.atDistanceAndAzimuth(halfWidthMeters, 90).atDistanceAndAzimuth(halfHeightMeters, 0)
        bottomLeftCoord      =  centerCoord.atDistanceAndAzimuth(halfWidthMeters, -90).atDistanceAndAzimuth(halfHeightMeters, 180)
        bottomRightCoord     =  centerCoord.atDistanceAndAzimuth(halfWidthMeters, 90).atDistanceAndAzimuth(halfHeightMeters, 180)
        console.log([topLeftCoord, topRightCoord, bottomRightCoord, bottomLeftCoord])
        return [topLeftCoord, topRightCoord, bottomRightCoord, bottomLeftCoord]
    }

    // Generate Automatic Geofence
    function optimalPolygonVertices() {

        var firstPoints = []
        var lastPoints = []
        var topCoordList = []
        var bottomCoordList = []
        var arrayPolygon = []
        var newPolyCoords = []
        var Mt = []
        var Ct = []
        var Mb = []
        var Cb = []
        var thetaList = []
        var Xt = []
        var Yt = []
        var Xb = []
        var Yb = []
        var appendedListx =[]
        var appendedListy =[]
        var X_Top = []
        var Y_Top = []
        var X_Bottom = []
        var Y_Bottom = []
        var coordinateListx = []
        var coordinateListy = []

        centerX = Math.round(mapControl.fromCoordinate(missionItems.get(1).coordinate,false).x)
        centerY = Math.round(mapControl.fromCoordinate(missionItems.get(1).coordinate,false).y)
        for (var r = 0; r <  missionItems.count - 2; r++) {
            var item = missionItems.get(r + 2)
            var point = mapControl.fromCoordinate(item.coordinate,false)
            coordinateListx.push(Math.round(point.x))
            coordinateListy.push(Math.round(point.y))
        }

        var lastcenterX = coordinateListx[coordinateListx.length - 1]
        var lastcenterY = coordinateListy[coordinateListy.length - 1]
        coordinateListx.splice(0, 0, centerX)
        coordinateListy.splice(0, 0, centerY)

        for (var m = 1; m < coordinateListx.length - 1; m++){
            appendedListx.push(coordinateListx[m])
            appendedListx.push(coordinateListx[m])
            appendedListy.push(coordinateListy[m])
            appendedListy.push(coordinateListy[m])
        }

        appendedListx.splice(0, 0, centerX)
        appendedListx.splice(appendedListx.length, 0, coordinateListx[coordinateListx.length - 1])
        appendedListy.splice(0, 0, centerY)
        appendedListy.splice(appendedListy.length, 0, coordinateListy[coordinateListy.length - 1])

        for (var n = 0; n < coordinateListx.length - 1; n++){

            var theta = Math.atan((coordinateListy[n + 1] - coordinateListy[n]) / (coordinateListx[n + 1] - coordinateListx[n]))

            if(coordinateListx[n + 1] > coordinateListx[n] && coordinateListy[n + 1] < coordinateListy[n] ){
                thetaList.push(theta)
            }

            else if(coordinateListx[n + 1] > coordinateListx[n] && coordinateListy[n + 1] > coordinateListy[n] && theta > 0){
                thetaList.push(theta)
            }

            else if(coordinateListx[n + 1] < coordinateListx[n] && coordinateListy[n + 1] > coordinateListy[n] && theta > 0){
                var newtheta1 = ((theta * (180 / Math.PI)) - 180) * (Math.PI / 180)
                thetaList.push(newtheta1)
            }

            else {
                var newtheta2 = ((theta * (180 / Math.PI)) + 180) * (Math.PI / 180)
                thetaList.push(newtheta2)
            }
        }

        for (var o = 0; o < appendedListx.length-1; o += 2){

                var xt1 = appendedListx[o] - 80 * Math.sin(thetaList[o / 2])
                var yt1 = appendedListy[o] + 80 * Math.cos(thetaList[o / 2])

                var xt2 = appendedListx[o + 1] - 80 * Math.sin(thetaList[o / 2])
                var yt2 = appendedListy[o + 1] + 80 * Math.cos(thetaList[o / 2])

                var xb1 = appendedListx[o] + 80 * Math.sin(thetaList[o / 2])
                var yb1 = appendedListy[o] - 80 * Math.cos(thetaList[o / 2])

                var xb2 = appendedListx[o+1] + 80 * Math.sin(thetaList[o / 2])
                var yb2 = appendedListy[o+1] - 80 * Math.cos(thetaList[o / 2])

                Xt.push(xt1)
                Xt.push(xt2)
                Xb.push(xb1)
                Xb.push(xb2)

                Yt.push(yt1)
                Yt.push(yt2)
                Yb.push(yb1)
                Yb.push(yb2)
        }

        for (var q = 0; q < Xt.length - 1; q += 2){

            var mt = (Yt[q] - Yt[q + 1]) / (Xt[q] - Xt[q + 1])
            var ct = (-mt * Xt[q + 1]) + Yt[q + 1]
            Mt.push(mt)
            Ct.push(ct)

            var mb = (Yb[q] - Yb[q + 1]) / (Xb[q] - Xb[q + 1])
            var cb = (-mb * Xb[q + 1]) + Yb[q + 1]
            Mb.push(mb)
            Cb.push(cb)
        }

        for (var p = 0; p < Mt.length - 1; p++){

            var xTop = (Ct[p + 1] - Ct[p]) / (Mt[p] - Mt[p + 1])
            var yTop = Mt[p] * xTop + Ct[p]

            var xBottom = (Cb[p + 1] - Cb[p]) / (Mb[p] - Mb[p + 1])
            var yBottom = Mb[p] * xBottom + Cb[p]

            X_Top.push(xTop)
            Y_Top.push(yTop)
            X_Bottom.push(xBottom)
            Y_Bottom.push(yBottom)
        }

        X_Top.splice(0, 0, Xt[0])
        Y_Top.splice(0, 0, Yt[0])

        X_Bottom.splice(0, 0, Xb[0])
        Y_Bottom.splice(0, 0, Yb[0])

        X_Top.splice(Xt.length - 1, 0, Xt[Xt.length - 1])
        Y_Top.splice(Yt.length - 1, 0, Yt[Yt.length - 1])

        X_Bottom.splice(Yb.length - 1, 0,  Xb[Xb.length - 1])
        Y_Bottom.splice(Yb.length - 1, 0, Yb[Yb.length - 1])


        for (var i = 90 + (thetaList[0] * (180 / Math.PI)); i <= 270+(thetaList[0] * (180 / Math.PI)); i += 9) {
            var anglef = i * Math.PI / 180.0
            var xf = centerX + radius * Math.cos(anglef)
            var yf = centerY + radius * Math.sin(anglef)
            firstPoints.push(mapControl.toCoordinate(Qt.point(xf, yf)))
        }

        for (var j = 270 + (thetaList[thetaList.length-1] * (180 / Math.PI)); j <= 450 + (thetaList[thetaList.length - 1]*(180/Math.PI)); j += 9) {
            var anglel = j * Math.PI / 180.0
            var xl = lastcenterX + radius * Math.cos(anglel)
            var yl = lastcenterY + radius * Math.sin(anglel)
            lastPoints.push(mapControl.toCoordinate(Qt.point(xl, yl)))
        }

        for (var k = 0; k <= X_Top.length - 1; k++) {
            var x_top = X_Top[k]
            var y_top = Y_Top[k]

            var x_bottom = X_Bottom[k]
            var y_bottom = Y_Bottom[k]

            var topCoord = mapControl.toCoordinate(Qt.point(x_top, y_top), false /* clipToViewPort */)
            var bottomCoord = mapControl.toCoordinate(Qt.point(x_bottom, y_bottom), false /* clipToViewPort */)
            topCoordList.push(topCoord)
            bottomCoordList.push(bottomCoord)
        }

        arrayPolygon.push(topCoordList)
        arrayPolygon.push(lastPoints.reverse())
        arrayPolygon.push(bottomCoordList.reverse())
        arrayPolygon.push(firstPoints.reverse())

        for (var l = 0; l < arrayPolygon.length; l++) {
            newPolyCoords = newPolyCoords.concat(arrayPolygon[l])
        }

        return newPolyCoords;
    }

    /// Reset polygon back to initial default
    function _resetPolygon() {
        mapPolygon.beginReset()
        mapPolygon.clear()
        mapPolygon.appendVertices(optimalPolygonVertices())
        mapPolygon.endReset()
    }

    function _handleInteractiveChanged() {
        if (interactive) {
            addEditingVisuals()
            addToolbarVisuals()
        } else {
            mapPolygon.traceMode = false
            removeEditingVisuals()
            removeToolVisuals()
        }
    }

    function _saveCurrentVertices() {
        _StoreCoordinates = [ ]
        for (var i=0; i<mapPolygon.count; i++) {
            _StoreCoordinates.push(mapPolygon.vertexCoordinate(i))
        }
    }

    function _restorePreviousVertices() {
        mapPolygon.beginReset()
        mapPolygon.clear()
        for (var i=0; i<_StoreCoordinates.length; i++) {
            mapPolygon.appendVertex(_StoreCoordinates[i])
        }
        mapPolygon.endReset()
    }

    onInteractiveChanged: _handleInteractiveChanged()


    Connections {
        target: mapPolygon
        onTraceModeChanged: {
            if (mapPolygon.traceMode) {
                _instructionText = _fenceText
                _objMgrTraceVisuals.createObject(traceMouseAreaComponent, mapControl, false)
            } else {
                _instructionText = _polygonToolsText
                _objMgrTraceVisuals.destroyObjects()
            }
        }
    }

    Component.onCompleted: {
        addCommonVisuals()
        _handleInteractiveChanged()
    }
    Component.onDestruction: mapPolygon.traceMode = false

    QGCDynamicObjectManager { id: _objMgrCommonVisuals }
    QGCDynamicObjectManager { id: _objMgrToolVisuals }
    QGCDynamicObjectManager { id: _objMgrEditingVisuals }
    QGCDynamicObjectManager { id: _objMgrTraceVisuals }
    QGCDynamicObjectManager { id: _objMgrCircleVisuals }

    QGCPalette { id: qgcPal }

    QGCMenu {
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

        QGCMenuItem {
            id:             removeVertexItem
            text:           qsTr("Remove vertex")
            onTriggered: {
                if (menu._editingVertexIndex >= 0) {
                    mapPolygon.removeVertex(menu._editingVertexIndex)
                }
            }
        }

        QGCMenuSeparator {
            visible:        removeVertexItem.visible
        }

        QGCMenuItem {
            text:           qsTr("Edit position..." )
            onTriggered:    editCenterPositionDialog.createObject(mainWindow).open()
        }

        QGCMenuItem {
            text:           qsTr("Edit position..." )
            visible:        menu._editingVertexIndex >= 0
            onTriggered:    editVertexPositionDialog.createObject(mainWindow).open()
        }
    }

    Component {
        id: polygonComponent

        MapPolygon {
            color:          mapPolygon.showAltColor ? altColor : interiorColor
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
            anchorPoint.x:  sourceItem.width / 2
            anchorPoint.y:  sourceItem.height / 2
            property int vertexIndex

            sourceItem: SplitIndicator {
                z:          _zorderSplitHandle
                onClicked:  if(_root.interactive) mapPolygon.splitPolygonSegment(mapQuickItem.vertexIndex)
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

            onClicked: if(_root.interactive) menu.popupVertex(polygonVertex)
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
            title:      qsTr("Edit Center Position")
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
            title:      qsTr("Edit Vertex Position")
            coordinate: mapPolygon.vertexCoordinate(menu._editingVertexIndex)
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

    Component {
        id: toolbarComponent

        PlanEditToolbar {
            anchors.horizontalCenter:       mapControl.left
            anchors.horizontalCenterOffset: mapControl.centerViewport.left + (mapControl.centerViewport.width / 2)
            y:                              mapControl.centerViewport.top
            availableWidth:                 mapControl.centerViewport.width

            QGCButton {
                _horizontalPadding: 2
                text:               qsTr("Automatic")
                visible:            !mapPolygon.traceMode
                onClicked:          _resetPolygon()
            }


            QGCButton {
                _horizontalPadding: 2
                text:               mapPolygon.traceMode ? qsTr("Done fencing") : qsTr("Mannual")
                onClicked: {
                    if (mapPolygon.traceMode) {
                        if (mapPolygon.count < 3) {
                            _restorePreviousVertices()
                        }
                        mapPolygon.traceMode = false
                    } else {
                        _saveCurrentVertices()
                        mapPolygon.traceMode = true
                        mapPolygon.clear()
                    }
                }
            }

        }
    }


    // Mouse area to capture clicks for tracing a polygon
    Component {
        id:  traceMouseAreaComponent

        MouseArea {
            anchors.fill:       mapControl
            z:                  QGroundControl.zOrderMapItems + 1   // Over item indicators

            onClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    mapPolygon.appendVertex(mapControl.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */))

                }
            }
        }
    }

    Component {
        id: radiusDragHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              QGroundControl.zOrderMapItems + 2

            sourceItem: Rectangle {
                id:         dragHandle
                width:      ScreenTools.defaultFontPixelHeight * 1.5
                height:     width
                radius:     width / 2
                color:      "white"
                opacity:    interiorOpacity * .90
            }
        }
    }
}
