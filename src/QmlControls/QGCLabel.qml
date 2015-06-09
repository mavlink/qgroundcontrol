import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Text {
    QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }

    property bool enabled: true

    font.pixelSize: ScreenTools.defaultFontPixelSize
    color:          __qgcPal.text
    antialiasing:   true
}
