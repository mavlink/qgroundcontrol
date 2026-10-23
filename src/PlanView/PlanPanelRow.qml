import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Selectable list row. Children are laid out in a row.
Rectangle {
    id: root

    default property alias rowContent: contentLayout.data

    property bool selected: false

    signal clicked

    implicitHeight: Math.max(ScreenTools.defaultFontPixelHeight * 2.2, ScreenTools.isMobile ? ScreenTools.minTouchPixels : 0)
    radius:         ScreenTools.defaultBorderRadius
    color:          selected ? qgcPal.accentSubtle : "transparent"
    border.width:   selected ? 1 : 0
    border.color:   qgcPal.accent

    QGCPalette { id: qgcPal }

    RowLayout {
        id: contentLayout

        anchors.fill:           parent
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        spacing:                ScreenTools.defaultFontPixelWidth
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      root.clicked()
    }
}
