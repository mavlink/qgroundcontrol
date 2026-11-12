import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Credit of what a map shows, in the bottom left corner of the map it is a child of
QGCMapLabel {
    /// The credit of each source of what the map shows. Empty ones are left out.
    property var credits: []
    /// Larger margins keep the credit clear of what a view puts in that corner, and of what it puts to the right of it
    property real leftMargin: _margin
    property real bottomMargin: _margin
    property real rightMargin: _margin

    readonly property real _margin: ScreenTools.defaultFontPixelWidth / 2

    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.leftMargin: leftMargin
    anchors.bottomMargin: bottomMargin
    width: Math.min(implicitWidth, Math.max(0, parent.width - leftMargin - rightMargin))
    text: credits.filter(credit => credit !== "").join(" | ")
    font.pointSize: ScreenTools.smallFontPointSize
    wrapMode: Text.WordWrap
    // The credits of an area can be many: the rest is cut off, so that the credit stays a small part of the map
    maximumLineCount: 2
    elide: Text.ElideRight
}
