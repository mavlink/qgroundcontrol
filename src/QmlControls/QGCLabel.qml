import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Text {
    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily
    color:          qgcPal.textPrimary
    antialiasing:   true

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
}
