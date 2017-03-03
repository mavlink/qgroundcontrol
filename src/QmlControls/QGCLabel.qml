import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Text {
    QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }

    property bool enabled: true

    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily
    color:          __qgcPal.text
    antialiasing:   true
}
