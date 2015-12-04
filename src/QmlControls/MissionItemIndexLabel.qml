import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette     1.0

Rectangle {
    property alias  label:          _label.text
    property bool   isCurrentItem:  false

    signal clicked

    readonly property real _margin: ScreenTools.defaultFontPixelWidth / 4

    QGCPalette { id: qgcPal }

    width:          _label.width + (_margin * 2)
    height:         _label.height +  (_margin * 2)
    radius:         _margin
    border.width:   1
    border.color:   "white"
    color:          isCurrentItem ? "green" : qgcPal.mapButtonHighlight

    MouseArea {
        anchors.fill:   parent
        onClicked:      parent.clicked()
    }

    QGCLabel {
        id:                     _label
        anchors.margins:        _margin
        anchors.left:           parent.left
        anchors.top:            parent.top
        color:                  "white"
        font.pixelSize:         ScreenTools.defaultFontPixelSize
    }
}
