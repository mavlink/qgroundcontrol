import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Small outlined pill for a short status such as "Shared"
Rectangle {
    id: control

    property alias text:    chipLabel.text
    property color textColor: qgcPal.textSecondary

    implicitWidth:  chipLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
    implicitHeight: chipLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.2
    radius:         height / 2
    color:          "transparent"
    border.color:   textColor

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    QGCLabel {
        id:                 chipLabel
        anchors.centerIn:   parent
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              control.textColor
    }
}
