/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtPositioning

import QGroundControl.GeoMap

/// Direction-indicator chevron positioned along a single fromCoord->toCoord
/// segment, at fraction of the way between them (screen-space, see
/// GeoMapMissionArrow). Self-contained (owns its own GeoMapProjectedPath)
/// for one-off segments.
Item {
    id: root

    property var scene
    property var surfaceModel
    property var fromCoord: QtPositioning.coordinate()
    property var toCoord: QtPositioning.coordinate()
    property real fraction: 0.75   ///< 0..1 along fromCoord -> toCoord

    GeoMapProjectedPath {
        id: segmentPath
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.Absolute
        coordinates: [root.fromCoord, root.toCoord]
    }

    readonly property bool _ready: segmentPath.projected && segmentPath.screenPoints.length > 1

    GeoMapMissionArrow {
        visible: root._ready
        screenPoint: root._ready
            ? Qt.point(segmentPath.screenPoints[0].x + (segmentPath.screenPoints[1].x - segmentPath.screenPoints[0].x) * root.fraction,
                       segmentPath.screenPoints[0].y + (segmentPath.screenPoints[1].y - segmentPath.screenPoints[0].y) * root.fraction)
            : Qt.point(0, 0)
        heading: root._ready
            ? Math.atan2(segmentPath.screenPoints[1].x - segmentPath.screenPoints[0].x,
                         -(segmentPath.screenPoints[1].y - segmentPath.screenPoints[0].y)) * 180 / Math.PI
            : 0
    }
}
