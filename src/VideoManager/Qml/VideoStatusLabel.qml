import QtQuick

import QGroundControl
import QGroundControl.Controls

// A bold status line over a video view, on a translucent backing.
Rectangle {
    id: root

    property alias text: label.text
    /// Small text, for a picture-in-picture view.
    property bool useSmallFont: true
    /// The widest the line gets, for a view narrower than its text, which then wraps.
    property real maximumWidth: Number.POSITIVE_INFINITY

    width:  label.width + ScreenTools.defaultFontPixelHeight
    height: label.contentHeight + ScreenTools.defaultFontPixelHeight
    radius: ScreenTools.defaultFontPixelWidth / 2
    color:  Qt.rgba(0, 0, 0, 0.5)

    QGCLabel {
        id:                     label
        anchors.centerIn:       parent
        width:                  Math.min(implicitWidth,
                                         Math.max(0, root.maximumWidth - ScreenTools.defaultFontPixelHeight))
        font.bold:              true
        color:                  "white"
        font.pointSize:         root.useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
        horizontalAlignment:    Text.AlignHCenter
        wrapMode:               Text.Wrap
    }
}
