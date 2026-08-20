/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl.Controls
import QGroundControl.GeoMap

/// Small height-value annotation anchored at a coordinate, top-center (label
/// grows down from the point) — a simplified GeoMap-native stand-in for
/// src/QmlControls/HeightIndicator.qml, which relies on QtLocation map-style
/// light/dark detection (QGCMapPalette/QGCMapLabel) not available on GeoMap.
GeoMapItem {
    id: root

    property alias text: label.text

    altitudeMode: GeoMapItem.ClampToGround
    anchorPoint: Qt.point(width / 2, 0)
    width: label.implicitWidth
    height: label.implicitHeight

    QGCLabel {
        id: label

        color: "white"
        font.bold: true
    }
}
