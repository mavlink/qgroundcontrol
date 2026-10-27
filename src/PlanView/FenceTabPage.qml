import QtQuick
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls

import "PlanPanelLogic.js" as Logic

/// GeoFence tab: add buttons, the list of fences with the breach return point, and the inspector of the selected entry.
/// The selected fence is the one being edited on the map.
ColumnLayout {
    id: root

    required property var planMasterController
    required property var editorMap

    property var selection: ({ "kind": "", "index": -1 })    ///< kind is "polygon", "circle", "breach" or empty

    readonly property string selectedKind:  selection.kind
    readonly property int   selectedIndex:  selection.index
    readonly property var   controller:     planMasterController.geoFenceController
    readonly property bool  supported:      controller.supported
    readonly property var   entries:        _buildEntries(controller.polygons.count, controller.circles.count)
    readonly property string selectedKey:   selectedKind === "breach" ? "breach" : (selectedKind === "" ? "" : selectedKind + "_" + selectedIndex)
    readonly property var   selectedObject: objectFor(selection.kind, selection.index)
    readonly property bool  hasSelection:   supported && (selection.kind === "breach" || objectFor(selection.kind, selection.index) !== null)
    readonly property int   selectedEntryIndex: entries.findIndex(entry => entry.key === selectedKey)

    function objectFor(kind, index) {
        if (kind === "polygon" && index >= 0 && index < controller.polygons.count) {
            return controller.polygons.get(index)
        }
        if (kind === "circle" && index >= 0 && index < controller.circles.count) {
            return controller.circles.get(index)
        }
        return null
    }

    function select(kind, index) {
        if (kind === selectedKind && index === selectedIndex) {
            return
        }
        selection = { "kind": kind, "index": index }
        controller.clearAllInteractive()
        const object = objectFor(kind, index)
        if (object) {
            object.interactive = true
        }
    }

    function clearSelection() {
        selection = { "kind": "", "index": -1 }
    }

    function _clearSelectionIfGone() {
        if ((selection.kind === "polygon" || selection.kind === "circle") && objectFor(selection.kind, selection.index) === null) {
            clearSelection()
        }
    }

    function selectEntry(entry) {
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        select(entry.kind, entry.index)
    }

    function step(delta) {
        const index = Logic.steppedIndex(selectedEntryIndex, delta, entries.length)
        if (index >= 0) {
            selectEntry(entries[index])
        }
    }

    function deleteSelected() {
        if (selectedKind === "polygon") {
            controller.deletePolygon(selectedIndex)
        } else if (selectedKind === "circle") {
            controller.deleteCircle(selectedIndex)
        }
        clearSelection()
    }

    function openInspector() {
        listInspector.openInspector()
    }

    function _buildEntries(polygonCount, circleCount) {
        const list = []
        for (let i = 0; i < polygonCount; i++) {
            list.push({ key: "polygon_" + i, kind: "polygon", index: i })
        }
        for (let i = 0; i < circleCount; i++) {
            list.push({ key: "circle_" + i, kind: "circle", index: i })
        }
        list.push({ key: "breach", kind: "breach", index: 0 })
        return list
    }

    // A GeoMap viewport corner can point into the sky in a 3D tilt: moves it toward the center until it meets the
    // ground, null if no point does
    function _groundCoordinateInViewport(corner, center) {
        for (let step = 0; step < 10; step++) {
            const fraction = step / 10
            const coordinate = editorMap.toCoordinate(
                Qt.point(corner.x + (center.x - corner.x) * fraction, corner.y + (center.y - corner.y) * fraction),
                false /* clipToViewPort */)
            if (coordinate.isValid) {
                return coordinate
            }
        }
        return null
    }

    function _normalizedLongitude(longitude) {
        let normalized = longitude
        while (normalized > 180) {
            normalized -= 360
        }
        while (normalized < -180) {
            normalized += 360
        }
        return normalized
    }

    // Whether point is inside the convex quadrilateral quad, edges included; a point sits on an edge within a tolerance
    // of about a centimeter (the points are {x: longitude offset, y: latitude} in degrees)
    function _insideQuadrilateral(point, quad) {
        const tolerance = 1e-7
        let side = 0
        for (let i = 0; i < quad.length; i++) {
            const from = quad[i]
            const to = quad[(i + 1) % quad.length]
            const edgeX = to.x - from.x
            const edgeY = to.y - from.y
            const edgeLength = Math.hypot(edgeX, edgeY)
            if (edgeLength === 0) {
                continue
            }
            const distance = (edgeX * (point.y - from.y) - edgeY * (point.x - from.x)) / edgeLength
            if (distance > tolerance) {
                if (side < 0) {
                    return false
                }
                side = 1
            } else if (distance < -tolerance) {
                if (side > 0) {
                    return false
                }
                side = -1
            }
        }
        return true
    }

    // Geographic bounds {north, south, west, east} of a rectangle, centered in the viewport, that lies within the
    // ground the viewport shows. The controller adds fences inside the bounds it is given, which the bounds of the
    // viewport corners are not once the map is rotated or tilted: they enclose the ground shown instead of lying
    // within it. The rectangle is shrunk from them until its corners are inside the quadrilateral of the corners'
    // ground. This only compares ground picks with each other, so it does not depend on the map projecting back to
    // the screen. Longitudes are taken relative to the viewport center so a viewport over the dateline works. Null if
    // the viewport shows no ground.
    function _viewportGroundBounds() {
        const viewport = editorMap.centerViewport
        const centerPoint = Qt.point(viewport.x + viewport.width / 2, viewport.y + viewport.height / 2)
        const corners = [
            Qt.point(viewport.x, viewport.y),
            Qt.point(viewport.x + viewport.width, viewport.y),
            Qt.point(viewport.x + viewport.width, viewport.y + viewport.height),
            Qt.point(viewport.x, viewport.y + viewport.height)
        ]
        const cornerCoordinates = []
        for (const corner of corners) {
            const coordinate = _groundCoordinateInViewport(corner, centerPoint)
            if (!coordinate) {
                return null
            }
            cornerCoordinates.push(coordinate)
        }
        // Relative to the center, a viewport of any width up to the whole world keeps its corners within a half
        // turn of it on either side
        const centerCoordinate = editorMap.toCoordinate(centerPoint, false /* clipToViewPort */)
        const referenceLongitude = centerCoordinate.isValid ? centerCoordinate.longitude : cornerCoordinates[0].longitude
        const quad = cornerCoordinates.map(
            coordinate => Qt.point(_normalizedLongitude(coordinate.longitude - referenceLongitude), coordinate.latitude))
        const minX = Math.min(...quad.map(point => point.x))
        const maxX = Math.max(...quad.map(point => point.x))
        const minY = Math.min(...quad.map(point => point.y))
        const maxY = Math.max(...quad.map(point => point.y))
        if (maxX - minX < 1e-9 || maxY - minY < 1e-9) {
            return null
        }

        const center = centerCoordinate.isValid ? Qt.point(0, centerCoordinate.latitude)
                                                : Qt.point((minX + maxX) / 2, (minY + maxY) / 2)
        const halfX = Math.max(maxX - center.x, center.x - minX)
        const halfY = Math.max(maxY - center.y, center.y - minY)

        for (let scale = 1; scale > 0.01; scale *= 0.9) {
            const west = center.x - halfX * scale
            const east = center.x + halfX * scale
            const south = center.y - halfY * scale
            const north = center.y + halfY * scale
            if ([Qt.point(west, south), Qt.point(east, south), Qt.point(east, north), Qt.point(west, north)]
                    .every(corner => _insideQuadrilateral(corner, quad))) {
                return {
                    north: Math.min(north, 90),
                    south: Math.max(south, -90),
                    west: _normalizedLongitude(referenceLongitude + west),
                    east: _normalizedLongitude(referenceLongitude + east)
                }
            }
        }
        return null
    }

    function _addFence(addFunction) {
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        const bounds = _viewportGroundBounds()
        if (!bounds) {
            return
        }
        addFunction(QtPositioning.coordinate(bounds.north, bounds.west), QtPositioning.coordinate(bounds.south, bounds.east))
    }

    objectName: "planPanel_page_fence"
    spacing:    ScreenTools.defaultFontPixelHeight * 0.3

    // A cleared or loaded plan empties the lists, so an index kept across it would select an unrelated fence
    Connections {
        target: root.controller.polygons

        function onCountChanged() {
            root._clearSelectionIfGone()
        }
    }

    Connections {
        target: root.controller.circles

        function onCountChanged() {
            root._clearSelectionIfGone()
        }
    }

    // The map can make a fence the interactive one as well
    Instantiator {
        model: root.controller.polygons

        delegate: QtObject {
            id: polygonTracker

            required property var object
            required property int index

            property Connections connection: Connections {
                target: polygonTracker.object

                function onInteractiveChanged() {
                    if (polygonTracker.object.interactive) {
                        root.select("polygon", polygonTracker.index)
                    }
                }
            }
        }

        onObjectAdded: (index, item) => {
            if (item.object.interactive) {
                root.select("polygon", index)
            }
        }
    }

    Instantiator {
        model: root.controller.circles

        delegate: QtObject {
            id: circleTracker

            required property var object
            required property int index

            property Connections connection: Connections {
                target: circleTracker.object

                function onInteractiveChanged() {
                    if (circleTracker.object.interactive) {
                        root.select("circle", circleTracker.index)
                    }
                }
            }
        }

        onObjectAdded: (index, item) => {
            if (item.object.interactive) {
                root.select("circle", index)
            }
        }
    }

    QGCLabel {
        objectName:         "planPanel_fenceUnsupported"
        Layout.fillWidth:   true
        Layout.margins:     ScreenTools.defaultFontPixelWidth
        visible:            !root.supported
        wrapMode:           Text.WordWrap
        text:               qsTr("This vehicle does not support GeoFence.")
    }

    RowLayout {
        Layout.fillWidth:       true
        Layout.leftMargin:      ScreenTools.defaultFontPixelWidth
        Layout.rightMargin:     ScreenTools.defaultFontPixelWidth
        Layout.topMargin:       ScreenTools.defaultFontPixelHeight * 0.3
        spacing:                ScreenTools.defaultFontPixelWidth
        visible:                root.supported

        QGCButton {
            objectName:         "planPanel_fenceAddPolygon"
            Layout.fillWidth:   true
            small:              true
            text:               qsTr("+ Polygon")
            onClicked:          root._addFence((topLeft, bottomRight) => root.controller.addInclusionPolygon(topLeft, bottomRight))
        }

        QGCButton {
            objectName:         "planPanel_fenceAddCircle"
            Layout.fillWidth:   true
            small:              true
            text:               qsTr("+ Circle")
            onClicked:          root._addFence((topLeft, bottomRight) => root.controller.addInclusionCircle(topLeft, bottomRight))
        }
    }

    PlanListInspector {
        id: listInspector

        objectName:         "planPanel_fenceListInspector"
        Layout.fillWidth:   true
        Layout.fillHeight:  true
        visible:            root.supported
        hasSelection:       root.hasSelection
        listComponent:      fenceListComponent
        inspectorComponent: fenceInspectorComponent
    }

    Component {
        id: fenceListComponent

        FenceItemList {
            page: root
        }
    }

    Component {
        id: fenceInspectorComponent

        FenceInspector {
            page:       root
            drillIn:    listInspector.layoutMode === "drillIn"
        }
    }
}
