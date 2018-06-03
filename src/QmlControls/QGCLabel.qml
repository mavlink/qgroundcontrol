import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Text {
    QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }

    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily
    color:          __qgcPal.text
    antialiasing:   true
}
