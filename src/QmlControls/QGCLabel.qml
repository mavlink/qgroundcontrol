import QtQuick                  2.12
import QtQuick.Controls         2.12

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Text {
    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily
    color:          qgcPal.text
    antialiasing:   true

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
}
