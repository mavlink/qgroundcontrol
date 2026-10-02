import QtQuick

// Rects of the upstream Fly View widgets which cover the map, in widget layer coordinates.
// A hidden widget collapses to a zero size rect at its anchor corner, so x/y stay usable for positioning.
QtObject {
    id: _root

    property rect pipView
    property rect toolStrip
    property rect topRight
    property rect bottomRight
    property rect mapScale
    property rect virtualJoystickLeft
    property rect virtualJoystickRight

    readonly property var types: [ "pipView", "toolStrip", "topRight", "bottomRight", "mapScale", "virtualJoystickLeft", "virtualJoystickRight" ]
    readonly property var all: types.map(type => _root[type]).filter(r => r.width > 0 && r.height > 0)
}
