import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette     1.0

Rectangle {
    property alias  label:          _label.text
    property bool   isCurrentItem:  false
    property bool   small:          false

    signal clicked

    width:          _width
    height:         _width
    radius:         _width / 2
    border.width:   small ? 1 : 2
    border.color:   "white"
    color:          isCurrentItem ? "green" : qgcPal.mapButtonHighlight

    property real _width: small ? ScreenTools.smallFontPixelSize * 1.5 : ScreenTools.mediumFontPixelSize * 1.5

    QGCPalette { id: qgcPal }

    MouseArea {
        anchors.fill: parent

        onClicked: parent.clicked()
    }

    QGCLabel {
        id:                     _label
        anchors.fill:           parent
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        color:                  "white"
        font.pixelSize:         small ? ScreenTools.smallFontPixelSize : ScreenTools.mediumFontPixelSize
    }
}
