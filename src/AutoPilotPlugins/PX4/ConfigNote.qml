import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Secondary text that wraps: row descriptions and cell notes. Hidden when empty.
QGCLabel {
    Layout.fillWidth:   true
    wrapMode:           Text.WordWrap
    font.pointSize:     ScreenTools.smallFontPointSize
    color:              qgcPal.textSecondary
    visible:            text !== ""

    QGCPalette { id: qgcPal }
}
