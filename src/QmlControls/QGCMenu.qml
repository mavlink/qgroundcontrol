import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

Menu {
    id: control

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    background: Rectangle {
        implicitWidth:  ScreenTools.defaultFontPixelWidth * 18
        color:          Qt.rgba(0.045, 0.048, 0.052, 0.98)
        radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.65)
        border.color:   Qt.rgba(0.82, 0.88, 0.94, 0.16)
        border.width:   1
    }

}
