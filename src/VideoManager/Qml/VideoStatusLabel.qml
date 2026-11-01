import QtQuick

import QGroundControl
import QGroundControl.Controls

// A bold status line over a video view, on a translucent backing.
Rectangle {
    id: root

    property alias text: label.text
    /// Small text, for a picture-in-picture view.
    property bool useSmallFont: true

    width:  label.contentWidth + ScreenTools.defaultFontPixelHeight
    height: label.contentHeight + ScreenTools.defaultFontPixelHeight
    radius: ScreenTools.defaultFontPixelWidth / 2
    color:  Qt.rgba(0, 0, 0, 0.5)

    QGCLabel {
        id:                 label
        anchors.centerIn:   parent
        font.bold:          true
        color:              "white"
        font.pointSize:     root.useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
    }
}
