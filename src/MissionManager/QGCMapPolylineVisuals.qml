/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtLocation                   5.3
import QtPositioning                5.3
import QtQuick.Dialogs              1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Palette           1.0
import QGroundControl.Controls          1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ShapeFileHelper   1.0

/// QGCMapPolyline map visuals
Item {
    id: _root

    property var    mapControl                  ///< Map control to place item in
    property var    mapPolyline                 ///< QGCMapPolyline object
    property bool   interactive:    mapPolyline.interactive
    property int    lineWidth:      3
    property color  lineColor:      "#be781c"

    property var    _dragHandlesComponent
    property var    _splitHandlesComponent
    property string _instructionText:       _corridorToolsText
    property real   _zorderDragHandle:      QGroundControl.zOrderMapItems + 3   // Highest to prevent splitting when items overlap
    property real   _zorderSplitHandle:     QGroundControl.zOrderMapItems + 2
    property var    _savedVertices:         [ ]

    readonly property string _corridorToolsText:    qsTr("Polyline Tools")
    readonly property string _traceText:            qsTr("Click in the map to add vertices. Click 'Done Tracing' when finished.")

    function _addCommonVisuals() {
        if (_objMgrCommonVisuals.empty) {
            _objMgrCommonVisuals.createObject(polylineComponent, mapControl, true)
        }
    }

    function _addInteractiveVisuals() {
        if (_objMgrInteractiveVisuals.empty) {
            _objMgrInteractiveVisuals.createObjects([ dragHandlesComponent, splitHandlesComponent, toolbarComponent ], mapControl)
        }
    }

    /// Calculate the default/initial polyline
    function _defaultPolylineVertices() {
        var x = mapControl.centerViewport.x + (mapControl.centerViewport.width / 2)
        var yInset = mapControl.centerViewport.height / 4
        var topPointCoord =     mapControl.toCoordinate(Qt.point(x, mapControl.centerViewport.y + yInset),                                false /* clipToViewPort */)
        var bottomPointCoord =  mapControl.toCoordinate(Qt.point(x, mapControl.centerViewport.y + mapControl.centerViewport.height - yInset),    false /* clipToViewPort */)
        return [ topPointCoord, bottomPointCoord ]
    }

    /// Reset polyline back to initial default
    function _resetPolyline() {
        mapPolyline.beginReset()
        mapPolyline.clear()
        var initialVertices = _defaultPolylineVertices()
        mapPolyline.appendVertex(initialVertices[0])
        mapPolyline.appendVertex(initialVertices[1])
        mapPolyline.endReset()
    }

    function _saveCurrentVertices() {
        _savedVertices = [ ]
        for (var i=0; i<mapPolyline.count; i++) {
            _savedVertices.push(mapPolyline.vertexCoordinate(i))
        }
    }

    function _restorePreviousVertices() {
        mapPolyline.beginReset()
        mapPolyline.clear()
        for (var i=0; i<_savedVertices.length; i++) {
            mapPolyline.appendVertex(_savedVertices[i])
        }
        mapPolyline.endReset()
    }

    onInteractiveChanged: {
        if (interactive) {
            _addInteractiveVisuals()
        } else {
            _objMgrInteractiveVisuals.destroyObjects()
        }
    }

    Connections {
        target: mapPolyline
        onTraceModeChanged: {
            if (mapPolyline.traceMode) {
                _instructionText = _traceText
                _objMgrTraceVisuals.createObject(traceMouseAreaComponent, mapControl, false)
            } else {
                _instructionText = _corridorToolsText
                _objMgrTraceVisuals.destroyObjects()
            }
        }
    }

    Component.onCompleted: {
        _addCommonVisuals()
        if (interactive) {
            _addInteractiveVisuals()
        }
    }
    Component.onDestruction: mapPolyline.traceMode = false

    QGCDynamicObjectManager { id: _objMgrCommonVisuals }
    QGCDynamicObjectManager { id: _objMgrInteractiveVisuals }
    QGCDynamicObjectManager { id: _objMgrTraceVisuals }

    QGCPalette { id: qgcPal }

    QGCFileDialog {
        id:             kmlLoadDialog
        folder:         QGroundControl.settingsManager.appSettings.missionSavePath
        title:          qsTr("Select KML File")
        selectExisting: true
        nameFilters:    ShapeFileHelper.fileDialogKMLFilters

        onAcceptedForLoad: {
            mapPolyline.loadKMLFile(file)
            close()
        }
    }

    QGCMenu {
        id: menu
        property int _removeVertexIndex

        function popUpWithIndex(curIndex) {
            _removeVertexIndex = curIndex
            removeVertexItem.visible = mapPolyline.count > 2
            menu.popup()
        }

        QGCMenuItem {
            id:             removeVertexItem
            text:           qsTr("Remove vertex" )
            onTriggered:    mapPolyline.removeVertex(menu._removeVertexIndex)
        }

        QGCMenuItem {
            text:           qsTr("Edit position..." )
            onTriggered:    mainWindow.showComponentDialog(editPositionDialog, qsTr("Edit Position"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel)
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
            visible:    _root.visible
            opacity:    _root.opacity
        }
    }

    Component {
        id: splitHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  sourceItem.width / 2
            anchorPoint.y:  sourceItem.height / 2
            z:              _zorderSplitHandle
            opacity:        _root.opacity

            property int vertexIndex

            sourceItem: SplitIndicator {
                onClicked:  mapPolyline.splitSegment(mapQuickItem.vertexIndex)
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

                opacity:    _root.opacity

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
            opacity:    _root.opacity

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
            opacity:        _root.opacity

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

                opacity:    _root.opacity

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

    Component {
        id: toolbarComponent

        PlanEditToolbar {
            anchors.horizontalCenter:       mapControl.left
            anchors.horizontalCenterOffset: mapControl.centerViewport.left + (mapControl.centerViewport.width / 2)
            y:                              mapControl.centerViewport.top
            z:                              QGroundControl.zOrderMapItems + 2
            availableWidth:                 mapControl.centerViewport.width

            QGCButton {
                _horizontalPadding: 0
                text:               qsTr("Basic")
                visible:            !mapPolyline.traceMode
                onClicked:          _resetPolyline()
            }

            QGCButton {
                _horizontalPadding: 0
                text:               mapPolyline.traceMode ? qsTr("Done Tracing") : qsTr("Trace")
                onClicked: {
                    if (mapPolyline.traceMode) {
                        if (mapPolyline.count < 2) {
                            _restorePreviousVertices()
                        }
                        mapPolyline.traceMode = false
                    } else {
                        _saveCurrentVertices()
                        mapPolyline.traceMode = true
                        mapPolyline.clear();
                    }
                }
            }

            QGCButton {
                _horizontalPadding: 0
                text:               qsTr("Load KML...")
                onClicked:          kmlLoadDialog.openForLoad()
                visible:            !mapPolyline.traceMode
            }
        }
    }

    // Mouse area to capture clicks for tracing a polyline
    Component {
        id:  traceMouseAreaComponent

        MouseArea {
            anchors.fill:       mapControl
            preventStealing:    true
            z:                  QGroundControl.zOrderMapItems + 1   // Over item indicators

            onClicked: {
                if (mouse.button === Qt.LeftButton && _root.interactive) {
                    mapPolyline.appendVertex(mapControl.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */))
                }
            }
        }
    }
}

