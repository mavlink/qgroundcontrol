/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Camera glyph used to mark camera trigger points on the map (both engines)
Rectangle {
    width: _radius * 2
    height: _radius * 2
    radius: _radius
    color: "black"
    opacity: 0.4

    readonly property real _radius: ScreenTools.defaultFontPixelHeight * 0.6

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCColoredImage {
        anchors.centerIn: parent
        anchors.alignWhenCentered: false // Prevents anchors.centerIn from snapping to integer coordinates, which can throw off centering.
        width: parent.width * 0.65
        height: width
        source: "/InstrumentValueIcons/camera.svg"
        sourceSize.height: height
        color: qgcPal.window
    }
}
