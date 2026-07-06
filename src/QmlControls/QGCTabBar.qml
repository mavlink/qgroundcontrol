import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

TabBar {
    background: Rectangle {
        color:        Qt.rgba(1, 1, 1, 0.026)
        radius:       Math.round(ScreenTools.defaultFontPixelWidth * 0.6)
        border.color: Qt.rgba(0.82, 0.88, 0.94, 0.12)
        border.width: 1
    }
}
