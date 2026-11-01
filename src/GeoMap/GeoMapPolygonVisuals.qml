pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.GeoMap

/// GeoMap counterpart of QGCMapPolygonVisuals: a QGCMapPolygon outline and,
/// while interactive, vertex and center drag handles and a split handle on
/// each edge to insert a vertex. Clicking a vertex or the center handle opens
/// its menu; edge lengths show while a vertex is dragged. With a mapView, an
/// interactive polygon also gets the polygon tools toolbar: start over with a
/// rectangle or a circle (dragged by its radius) in view, trace by clicking the
/// map, or load a KML/SHP file.
Item {
    id: root

    property var scene
    property var surfaceModel
    property var mapPolygon                 ///< QGCMapPolygon object
    property bool interactive: mapPolygon ? mapPolygon.interactive : false
    property color borderColor: QGroundControl.globalPalette.mapMissionTrajectory
    property real borderWidth: 3
    property color fillColor: "transparent"
    property real extrudeHeightMeters: 0    ///< 3D fence wall height, see GeoMapPolygon
    property color extrudeColor: Qt.alpha(borderColor, 0.25)
    property real node3DOpacity: 1          ///< Dimming of the wall, see GeoMapItem.node3DOpacity
    property var mapView                    ///< GeoMapView hosting the toolbar and trace clicks; no tools without it

    readonly property bool _dragging: mapPolygon ? (mapPolygon.vertexDrag || mapPolygon.centerDrag) : false
    // While dragging the polygon only notifies dragPath
    readonly property var _path: mapPolygon ? (_dragging ? mapPolygon.dragPath : mapPolygon.path) : []
    property bool _vertexBeingDragged: false
    readonly property bool _traceMode: mapPolygon ? mapPolygon.traceMode : false
    property bool _circleMode: false
    property real _circleRadius: 0
    property bool _circleRadiusDrag: false
    property var _circleRadiusDragCoordinate: QtPositioning.coordinate()
    property var _circleRadiusDragCenter: QtPositioning.coordinate()
    property var _savedVertices: []
    property bool _savedCircleMode: false

    function _popupMenu(vertexIndex) {
        menu.vertexIndex = vertexIndex
        menu.popup()
    }

    function _replaceVertices(vertices) {
        mapPolygon.beginReset()
        mapPolygon.clear()
        mapPolygon.appendVertices(vertices)
        mapPolygon.endReset()
    }

    /// Center and half sizes of the shape Basic and Circular start over with, the same as a new fence's: three quarters
    /// of the ground in view, at most 3000 meters across. Null while the map shows no ground.
    function _defaultShape() {
        const bounds = MapViewportTools.groundBounds(mapView)
        if (!bounds) {
            return null
        }
        const topLeft = QtPositioning.coordinate(bounds.north, bounds.west)
        const halfWidth = topLeft.distanceTo(QtPositioning.coordinate(bounds.north, bounds.east)) / 2
        const halfHeight = topLeft.distanceTo(QtPositioning.coordinate(bounds.south, bounds.west)) / 2
        return {
            center: QtPositioning.coordinate(topLeft.atDistanceAndAzimuth(halfHeight, 180).latitude,
                                             topLeft.atDistanceAndAzimuth(halfWidth, 90).longitude),
            halfWidth: Math.min(halfWidth * 0.75, 1500),
            halfHeight: Math.min(halfHeight * 0.75, 1500)
        }
    }

    function _resetPolygon() {
        const shape = _defaultShape()
        if (!shape) {
            return
        }
        const corner = (eastWest, northSouth) => shape.center.atDistanceAndAzimuth(shape.halfWidth, eastWest)
                                                             .atDistanceAndAzimuth(shape.halfHeight, northSouth)
        _circleMode = false
        _replaceVertices([ corner(-90, 0), corner(90, 0), corner(90, 180), corner(-90, 180) ])
    }

    function _resetCircle() {
        const shape = _defaultShape()
        if (shape) {
            _createCircle(shape.center, Math.min(shape.halfWidth, shape.halfHeight))
        }
    }

    function _createCircle(center, radius) {
        const segments = 16
        const vertices = []
        for (let i = 0; i < segments; i++) {
            vertices.push(center.atDistanceAndAzimuth(radius, i * 360 / segments))
        }
        _circleRadius = radius
        _circleMode = true
        _replaceVertices(vertices)
    }

    /// Moves the circle's vertices in place: while dragging, rebuilding the polygon would recreate its vertices and
    /// notify a full path change on every move
    function _resizeCircle(center, radius) {
        const segments = mapPolygon.count
        for (let i = 0; i < segments; i++) {
            mapPolygon.adjustVertex(i, center.atDistanceAndAzimuth(radius, i * 360 / segments))
        }
        _circleRadius = radius
    }

    function _toggleTrace() {
        if (_traceMode) {
            if (mapPolygon.count < 3) {
                _replaceVertices(_savedVertices)
                _circleMode = _savedCircleMode
            }
            mapPolygon.traceMode = false
        } else {
            _savedVertices = []
            for (let i = 0; i < mapPolygon.count; i++) {
                _savedVertices.push(mapPolygon.vertexCoordinate(i))
            }
            _savedCircleMode = _circleMode
            _circleMode = false
            mapPolygon.traceMode = true
            mapPolygon.clear()
        }
    }

    // Tracing ends with the editing
    onInteractiveChanged: {
        if (!interactive && mapPolygon && mapPolygon.traceMode) {
            _toggleTrace()
        }
    }
    Component.onDestruction: {
        if (mapPolygon) {
            mapPolygon.traceMode = false
        }
    }

    GeoMapPolygon {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinates: root._path
        strokeColor: root.borderColor
        strokeWidth: root.borderWidth
        fillColor: root.fillColor
        extrudeHeightMeters: root.extrudeHeightMeters
        extrudeColor: root.extrudeColor
        node3DOpacity: root.node3DOpacity
    }

    Repeater {
        model: (root.interactive && !root._dragging && !root._circleMode) ? root._path : []

        GeoMapItem {
            id: splitHandle
            objectName: "geoMapPolygonSplitHandle"

            required property var modelData
            required property int index

            readonly property var _nextVertex: root._path[(index + 1) % root._path.length]

            scene: root.scene
            surfaceModel: root.surfaceModel
            coordinate: modelData.atDistanceAndAzimuth(modelData.distanceTo(_nextVertex) / 2, modelData.azimuthTo(_nextVertex))
            width: splitIndicator.width
            height: splitIndicator.height
            anchorPoint: Qt.point(width / 2, height / 2)

            SplitIndicator {
                id: splitIndicator
                onClicked: root.mapPolygon.splitPolygonSegment(splitHandle.index)
            }
        }
    }

    Repeater {
        model: root._vertexBeingDragged ? root._path : []

        GeoMapItem {
            id: edgeLength
            objectName: "geoMapPolygonEdgeLength"

            required property var modelData
            required property int index

            readonly property var _nextVertex: root._path[(index + 1) % root._path.length]
            readonly property real distance: modelData.distanceTo(_nextVertex)

            scene: root.scene
            surfaceModel: root.surfaceModel
            coordinate: modelData.atDistanceAndAzimuth(distance / 2, modelData.azimuthTo(_nextVertex))
            width: edgeLengthLabel.implicitWidth
            height: edgeLengthLabel.implicitHeight
            anchorPoint: Qt.point(width / 2, height / 2)

            QGCLabel {
                id: edgeLengthLabel
                color: "white"
                style: Text.Outline
                styleColor: "black"
                text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(edgeLength.distance).toFixed(1) + " "
                      + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
            }
        }
    }

    Repeater {
        model: (root.mapPolygon && root.interactive && !root._circleMode) ? root.mapPolygon.pathModel : undefined

        GeoMapDragHandle {
            objectName: "geoMapPolygonVertexHandle"

            required property var object
            required property int index

            scene: root.scene
            surfaceModel: root.surfaceModel
            coordinate: object.coordinate
            visible: !root.mapPolygon.centerDrag

            onDragStarted: {
                root._vertexBeingDragged = true
                root.mapPolygon.vertexDrag = true
            }
            onClicked: root._popupMenu(index)
            onDragged: (coordinate) => root.mapPolygon.adjustVertex(index, QtPositioning.coordinate(coordinate.latitude, coordinate.longitude))
            onDragFinished: {
                root._vertexBeingDragged = false
                root.mapPolygon.vertexDrag = false
                root.mapPolygon.verifyClockwiseWinding()
            }
        }
    }

    GeoMapDragHandle {
        objectName: "geoMapPolygonCenterHandle"
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root.mapPolygon ? (root.mapPolygon.centerDrag ? root.mapPolygon.dragCenter : root.mapPolygon.center) : QtPositioning.coordinate()
        visible: root.interactive && !root._vertexBeingDragged

        onDragStarted: {
            root.mapPolygon.centerDrag = true
            root.mapPolygon.vertexDrag = true
        }
        onClicked: root._popupMenu(-1)
        onDragged: (coordinate) => root.mapPolygon.center = QtPositioning.coordinate(coordinate.latitude, coordinate.longitude)
        onDragFinished: {
            root.mapPolygon.centerDrag = false
            root.mapPolygon.vertexDrag = false
        }

        QGCColoredImage {
            anchors.fill: parent
            color: "black"
            mipmap: true
            fillMode: Image.PreserveAspectFit
            source: "/qmlimages/MapCenter.svg"
            sourceSize.height: height
        }
    }

    GeoMapDragHandle {
        id: radiusHandle
        objectName: "geoMapPolygonRadiusHandle"
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._circleRadiusDrag
                    ? root._circleRadiusDragCoordinate
                    : (root.mapPolygon ? root.mapPolygon.center.atDistanceAndAzimuth(root._circleRadius, 90) : QtPositioning.coordinate())
        visible: root.interactive && root._circleMode && !(root.mapPolygon && root.mapPolygon.centerDrag)

        onDragStarted: {
            // The binding switches to the drag coordinate: start it where the handle is
            root._circleRadiusDragCoordinate = radiusHandle.coordinate
            // The polygon recomputes its center from the moved vertices; the circle keeps the one it started with
            root._circleRadiusDragCenter = root.mapPolygon.center
            root._circleRadiusDrag = true
            root.mapPolygon.vertexDrag = true
        }
        onDragged: (coordinate) => {
            root._circleRadiusDragCoordinate = coordinate
            root._resizeCircle(root._circleRadiusDragCenter, root._circleRadiusDragCenter.distanceTo(coordinate))
        }
        onDragFinished: {
            root._circleRadiusDrag = false
            root.mapPolygon.vertexDrag = false
        }
    }

    Connections {
        target: root.mapView
        enabled: root.interactive && root._traceMode

        function onMapClicked(position) {
            const coordinate = root.mapView.toCoordinate(position, false /* clipToViewPort */)
            if (coordinate.isValid) {
                root.mapPolygon.appendVertex(coordinate)
            }
        }
    }

    Rectangle {
        id: toolbar
        objectName: "geoMapPolygonToolbar"
        parent: root.mapView
        visible: root.interactive && !!root.mapView
        z: QGroundControl.zOrderWidgets
        x: _viewport.x + (_viewport.width - width) / 2
        y: _viewport.y + _margins
        width: Math.min(toolbarRow.implicitWidth + (_margins * 2), _viewport.width)
        height: toolbarRow.implicitHeight + (_margins * 2)
        radius: _margins
        color: QGroundControl.globalPalette.toolbarBackground

        readonly property real _margins: ScreenTools.defaultFontPixelWidth / 2
        readonly property rect _viewport: root.mapView ? MapViewportTools.viewportRect(root.mapView) : Qt.rect(0, 0, 0, 0)

        // Scrolls sideways when the map is too narrow for the tools
        QGCFlickable {
            objectName: "geoMapPolygonToolbarFlickable"
            x: toolbar._margins
            y: toolbar._margins
            width: toolbar.width - (toolbar._margins * 2)
            height: toolbarRow.implicitHeight
            flickableDirection: Flickable.HorizontalFlick
            contentWidth: toolbarRow.implicitWidth
            contentHeight: height

            RowLayout {
                id: toolbarRow
                spacing: toolbar._margins

                QGCButton {
                    objectName: "geoMapPolygonBasic"
                    text: qsTr("Basic")
                    visible: !root._traceMode
                    onClicked: root._resetPolygon()
                }

                QGCButton {
                    objectName: "geoMapPolygonCircular"
                    text: qsTr("Circular")
                    visible: !root._traceMode
                    onClicked: root._resetCircle()
                }

                QGCButton {
                    objectName: "geoMapPolygonTrace"
                    text: root._traceMode ? qsTr("Done Tracing") : qsTr("Trace")
                    onClicked: root._toggleTrace()
                }

                QGCButton {
                    objectName: "geoMapPolygonLoadFile"
                    text: qsTr("Load KML/SHP...")
                    visible: !root._traceMode
                    onClicked: kmlOrSHPLoadDialog.openForLoad()
                }

                QGCLabel {
                    text: root._traceMode ? qsTr("Click in the map to add vertices. Click 'Done Tracing' when finished.")
                                          : qsTr("Polygon Tools")
                }
            }
        }
    }

    KMLOrSHPFileDialog {
        id: kmlOrSHPLoadDialog
        title: qsTr("Select Polygon File")

        onAcceptedForLoad: (file) => {
            if (root.mapPolygon.loadKMLOrSHPFile(file)) {
                root._circleMode = false
                root.mapView.setVisibleRegion(QtPositioning.rectangle(root.mapPolygon.path))
            }
            close()
        }
    }

    QGCMenu {
        id: menu
        objectName: "geoMapPolygonMenu"

        property int vertexIndex: -1    ///< -1: the center handle's menu

        QGCMenuItem {
            id: removeVertexItem
            objectName: "geoMapPolygonRemoveVertex"
            text: qsTr("Remove vertex")
            visible: menu.vertexIndex >= 0 && root.mapPolygon && root.mapPolygon.count > 3
            onTriggered: root.mapPolygon.removeVertex(menu.vertexIndex)
        }

        QGCMenuSeparator {
            visible: removeVertexItem.visible
        }

        QGCMenuItem {
            objectName: "geoMapPolygonSetRadius"
            text: qsTr("Set radius...")
            visible: menu.vertexIndex < 0 && root._circleMode
            onTriggered: setRadiusDialogFactory.open()
        }

        QGCMenuItem {
            objectName: "geoMapPolygonEditPosition"
            text: qsTr("Edit position...")
            onTriggered: {
                // The dialog starts at the position instead of binding to it: it writes the position back
                if (menu.vertexIndex >= 0) {
                    editVertexPositionDialogFactory.open({ vertexIndex: menu.vertexIndex,
                                                           coordinate: root.mapPolygon.vertexCoordinate(menu.vertexIndex) })
                } else {
                    editCenterPositionDialogFactory.open({ coordinate: root.mapPolygon.center })
                }
            }
        }
    }

    QGCPopupDialogFactory {
        id: editVertexPositionDialogFactory

        dialogComponent: editVertexPositionDialog
    }

    Component {
        id: editVertexPositionDialog

        EditPositionDialog {
            objectName: "geoMapPolygonVertexPositionDialog"
            title: qsTr("Edit Vertex Position")

            // Set with the coordinate when the dialog opens, which writes to this vertex
            required property int vertexIndex

            onCoordinateChanged: {
                root.mapPolygon.adjustVertex(vertexIndex, coordinate)
                root.mapPolygon.verifyClockwiseWinding()
            }
        }
    }

    QGCPopupDialogFactory {
        id: editCenterPositionDialogFactory

        dialogComponent: editCenterPositionDialog
    }

    Component {
        id: editCenterPositionDialog

        EditPositionDialog {
            objectName: "geoMapPolygonCenterPositionDialog"
            title: qsTr("Edit Center Position")

            onCoordinateChanged: {
                // Moving the vertices one by one would notify a path change per vertex
                root.mapPolygon.centerDrag = true
                root.mapPolygon.center = coordinate
                root.mapPolygon.centerDrag = false
            }
        }
    }

    QGCPopupDialogFactory {
        id: setRadiusDialogFactory

        dialogComponent: setRadiusDialog
    }

    Component {
        id: setRadiusDialog

        QGCPopupDialog {
            objectName: "geoMapPolygonSetRadiusDialog"
            title: qsTr("Set Radius")
            buttons: Dialog.Save | Dialog.Cancel

            onAccepted: {
                const radius = Number(radiusField.text)
                if (!isNaN(radius) && radius > 0) {
                    root._createCircle(root.mapPolygon.center,
                                       QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsToMeters(radius))
                } else {
                    preventClose = true
                }
            }

            ColumnLayout {
                width: ScreenTools.defaultFontPixelWidth * 30
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("Enter circle radius.")
                    wrapMode: Text.WordWrap
                }

                QGCTextField {
                    id: radiusField
                    objectName: "geoMapPolygonRadiusField"
                    Layout.fillWidth: true
                    text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(root._circleRadius).toFixed(1)
                    // Same number syntax as toFixed() writes and Number() reads
                    validator: DoubleValidator { bottom: 0.1; notation: DoubleValidator.StandardNotation; locale: "C" }
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
                }
            }
        }
    }
}
