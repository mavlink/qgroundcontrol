pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// The video sources that play beside the main picture, in a row of tiles: the USB camera or the stream it took the main
// picture from, then the other streams of the vehicle's cameras. The tiles share the width the row is given, which the
// row tells the manager the room of: the manager plays no more sources in tiles than fit at a tile's least size, and
// none before a row has told.
Row {
    id: root

    /// The manager whose sources in tiles are shown.
    property VideoManager videoManager: QGroundControl.videoManager
    /// The width the row may take.
    property real maximumWidth: 0
    /// Whether the tiles are on display: not in a view without room for them, whose sources then play nowhere.
    property bool shown: true
    /// How long, in milliseconds, a row that is no longer shown leaves the manager with the room it had.
    property int hiddenGrace: root.videoManager.idleSourceGraceMs

    /// The least width of a tile: a smaller picture shows nothing.
    readonly property real minimumTileWidth: ScreenTools.defaultFontPixelHeight * 4 * root._tileAspectRatio

    readonly property int _tileCount: root.videoManager.tileCount
    // Wide-screen tiles, small enough to leave the main picture in view, and smaller where the row has less room.
    readonly property real _tileAspectRatio: 16 / 9
    readonly property int _room: Math.max(0, Math.floor((root.maximumWidth + root.spacing)
                                                        / (root.minimumTileWidth + root.spacing)))
    readonly property real _tileWidth: root._tileCount === 0 ? 0
        : Math.max(0, Math.min(ScreenTools.defaultFontPixelHeight * 8 * root._tileAspectRatio,
                               (root.maximumWidth - (root.spacing * (root._tileCount - 1))) / root._tileCount))

    spacing: ScreenTools.defaultFontPixelWidth
    visible: root.shown

    // A row that isn't shown, as the picture-in-picture's, leaves the manager with the room the row had: the tiles'
    // sources stay as they are for a row that is soon back, as when the video changes places with the map and back.
    Binding {
        target:         root.videoManager
        property:       "tileRoom"
        value:          root._room
        when:           root.shown
        restoreMode:    Binding.RestoreNone
    }

    // Out of view for longer, the row has no room: the tiles' sources go, so that nothing is fetched for tiles nobody
    // sees.
    Timer {
        interval:       root.hiddenGrace
        running:        !root.shown
        onTriggered:    root.videoManager.tileRoom = 0
    }

    Repeater {
        model: root.videoManager.sources

        Loader {
            id: tileLoader

            required property VideoStreamController modelData

            active:     tileLoader.modelData.tile
            // The loader of a source that has no tile would still take a place in the row.
            visible:    tileLoader.modelData.tile

            sourceComponent: VideoTile {
                source:         tileLoader.modelData
                videoManager:   root.videoManager
                shown:          root.shown
                width:          root._tileWidth
                height:         width / root._tileAspectRatio
            }
        }
    }
}
