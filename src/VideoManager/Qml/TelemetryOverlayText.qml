import QtQuick

import QGroundControl
import QGroundControl.Controls

// The live telemetry overlay: the telemetry bar's values along the bottom of the picture this item is placed in,
// wrapped where the picture is narrower. An item over the video, not part of its frames: it follows the telemetry while
// the video is frozen or slow, and stays upright over a turned or mirrored picture.
Rectangle {
    id: root

    property alias text: label.text
    /// Small text, for a picture-in-picture view.
    property bool useSmallFont: true

    readonly property real _margin: ScreenTools.defaultFontPixelHeight / 2
    // The picture's width less a margin on each side of the backing, and of the text in it.
    readonly property real _maximumTextWidth: root.parent ? Math.max(0, root.parent.width - (4 * root._margin)) : 0

    width:      label.width + (2 * root._margin)
    height:     label.height + root._margin
    radius:     ScreenTools.defaultFontPixelWidth / 2
    color:      Qt.rgba(0, 0, 0, 0.5)
    visible:    root.text !== ""

    anchors {
        horizontalCenter:   root.parent ? root.parent.horizontalCenter : undefined
        bottom:             root.parent ? root.parent.bottom : undefined
        bottomMargin:       root._margin
    }

    QGCLabel {
        id:                     label
        anchors.centerIn:       parent
        width:                  Math.min(implicitWidth, root._maximumTextWidth)
        color:                  "white"
        font.pointSize:         root.useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.mediumFontPointSize
        horizontalAlignment:    Text.AlignHCenter
        wrapMode:               Text.Wrap
    }
}
