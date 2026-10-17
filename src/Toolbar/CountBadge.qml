import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Round badge showing a count. Counts above 9 show as "9+".
Rectangle {
    id:         control
    width:      Math.max(height, countLabel.implicitWidth + ScreenTools.defaultFontPixelWidth / 2)
    height:     ScreenTools.defaultFontPixelHeight
    radius:     height / 2
    visible:    count > 0

    property int count: 0

    readonly property string countText: count > 9 ? "9+" : count.toString()

    QGCLabel {
        id:                 countLabel
        anchors.centerIn:   parent
        // Superscript "+" keeps the capped count inside the circle
        text:               control.count > 9 ? "9<sup>+</sup>" : control.countText
        textFormat:         Text.RichText
        color:              QGroundControl.globalPalette.indicatorText
        font.pointSize:     ScreenTools.smallFontPointSize
        font.bold:          true
    }
}
