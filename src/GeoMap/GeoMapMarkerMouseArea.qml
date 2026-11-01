import QtQuick

/// Click and drag handling shared by the GeoMap point markers: a press that stays under the drag threshold is a click,
/// past it a draggable marker reports where the cursor drags it to.
MouseArea {
    id: root

    required property var marker    ///< GeoMapItem this area belongs to
    property bool draggable: false  ///< The marker can be dragged; the map is not panned meanwhile

    // Cursor-to-anchor offset at press, so the marker doesn't jump to the cursor
    property point _grabOffset
    property point _pressPosition
    property bool _moved: false

    signal markerClicked()
    signal dragged(point target)    ///< Where the marker's anchor is dragged to, in the marker's parent coordinates
    signal dragFinished()

    function _parentPosition(mouse) {
        return root.marker.mapToItem(root.marker.parent, mouse.x, mouse.y)
    }

    // The map pan DragHandler would otherwise take over past the drag threshold
    preventStealing: root.draggable

    onPressed: (mouse) => {
        // Modified drags are the map's orbit and look gestures
        if (mouse.modifiers !== Qt.NoModifier) {
            mouse.accepted = false
            return
        }
        const pos = _parentPosition(mouse)
        _pressPosition = pos
        _grabOffset = Qt.point(root.marker.x + root.marker.anchorPoint.x - pos.x,
                               root.marker.y + root.marker.anchorPoint.y - pos.y)
        _moved = false
    }

    onPositionChanged: (mouse) => {
        if (!root.draggable || !pressed) {
            return
        }
        const pos = _parentPosition(mouse)
        if (!_moved && Math.hypot(pos.x - _pressPosition.x, pos.y - _pressPosition.y) < Qt.styleHints.startDragDistance) {
            return
        }
        _moved = true
        root.dragged(Qt.point(pos.x + _grabOffset.x, pos.y + _grabOffset.y))
    }

    onReleased: {
        if (_moved) {
            root.dragFinished()
        }
    }

    // A drag cut short (the grab taken away, the item hidden) still ends
    onCanceled: {
        if (_moved) {
            _moved = false
            root.dragFinished()
        }
    }

    onClicked: {
        if (!_moved) {
            root.markerClicked()
        }
    }
}
