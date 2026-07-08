import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

MenuItem {
    id: control

    // MenuItem doesn't support !visible so we have to hack it in
    height: visible ? implicitHeight : 0

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    contentItem: Text {
        text:               control.text
        font.family:        ScreenTools.normalFontFamily
        font.pointSize:     ScreenTools.controlFontPointSize
        color:              control.highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText
        verticalAlignment:  Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth:  ScreenTools.defaultFontPixelWidth * 18
        implicitHeight: Math.round(ScreenTools.defaultFontPixelHeight * 1.9)
        color:          control.highlighted ? Qt.rgba(0.18, 0.20, 0.22, 0.98) : "transparent"
        radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.45)
    }
}
