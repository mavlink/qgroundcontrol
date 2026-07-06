/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl.ScreenTools

Item {
    id: frame

    property real navigationWidth:       0
    property real contentLeft:           navigationWidth
    property real outerMargin:           ScreenTools.defaultFontPixelHeight * 0.50
    property real paneGap:               ScreenTools.defaultFontPixelWidth * 0.65
    property real cornerRadius:          Math.round(ScreenTools.defaultFontPixelWidth * 0.78)
    property color pageColor:            Qt.rgba(0.060, 0.063, 0.067, 1.0)
    property color navigationColor:      Qt.rgba(1, 1, 1, 0.036)
    property color contentColor:         Qt.rgba(1, 1, 1, 0.018)
    property color dividerColor:         Qt.rgba(0.82, 0.88, 0.94, 0.045)

    Rectangle {
        anchors.fill: parent
        color:        frame.pageColor
    }

    Rectangle {
        x:      frame.outerMargin
        y:      frame.outerMargin
        width:  Math.max(0, frame.navigationWidth - frame.outerMargin - frame.paneGap * 0.5)
        height: Math.max(0, parent.height - frame.outerMargin * 2)
        radius: frame.cornerRadius
        color:  frame.navigationColor
    }

    Rectangle {
        x:      Math.min(parent.width, Math.max(frame.outerMargin, frame.contentLeft + frame.paneGap))
        y:      frame.outerMargin
        width:  Math.max(0, parent.width - x - frame.outerMargin)
        height: Math.max(0, parent.height - frame.outerMargin * 2)
        radius: frame.cornerRadius
        color:  frame.contentColor
    }

    Rectangle {
        x:      Math.max(0, frame.contentLeft)
        y:      frame.outerMargin * 1.35
        width:  1
        height: Math.max(0, parent.height - frame.outerMargin * 2.7)
        color:  frame.dividerColor
    }
}
