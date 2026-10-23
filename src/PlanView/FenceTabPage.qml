import QtQuick
import QtQuick.Layouts

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

    function _addFence(addFunction) {
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        const viewport = editorMap.centerViewport
        const topLeft = editorMap.toCoordinate(Qt.point(viewport.x, viewport.y), false /* clipToViewPort */)
        const bottomRight = editorMap.toCoordinate(Qt.point(viewport.x + viewport.width, viewport.y + viewport.height), false /* clipToViewPort */)
        addFunction(topLeft, bottomRight)
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
