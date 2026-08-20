/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Shapes

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Google Maps-style teardrop marker anchored at its tip. Shows an inner dot
/// by default, or a short label when one is set.
GeoMapItem {
    id: root

    property color color: qgcPal.mapIndicator
    property alias label: labelText.text

    readonly property real _headRadius: width / 2

    // Tangent points where straight tail lines touch the head circle, so the
    // arc flows into the tail without a corner. Clamped: height <= 2 * head
    // radius has no tangent solution and would produce NaN geometry.
    readonly property real _tangentAngle: Math.acos(Math.min(1, _headRadius / (height - _headRadius)))
    readonly property real _tangentDX: _headRadius * Math.sin(_tangentAngle)
    readonly property real _tangentY: _headRadius + _headRadius * Math.cos(_tangentAngle)

    width: ScreenTools.defaultFontPixelHeight * 1.25
    height: width * 1.4
    anchorPoint: Qt.point(width / 2, height)

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: -1
            fillColor: root.color

            startX: root.width / 2
            startY: root.height

            PathLine {
                x: root.width / 2 - root._tangentDX
                y: root._tangentY
            }
            PathArc {
                x: root.width / 2 + root._tangentDX
                y: root._tangentY
                radiusX: root._headRadius
                radiusY: root._headRadius
                useLargeArc: true
                direction: PathArc.Clockwise
            }
            PathLine {
                x: root.width / 2
                y: root.height
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root._headRadius - height / 2
        width: root._headRadius * 0.8
        height: width
        radius: width / 2
        color: Qt.darker(root.color, 1.5)
        visible: labelText.text === ""
    }

    QGCLabel {
        id: labelText

        anchors.horizontalCenter: parent.horizontalCenter
        y: root._headRadius - height / 2
        color: "white"
        font.bold: true
    }
}
