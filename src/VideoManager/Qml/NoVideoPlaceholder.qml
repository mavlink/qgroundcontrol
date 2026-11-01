import QtQuick

import QGroundControl
import QGroundControl.Controls

// What a video view shows without video: whether the stream is awaited or disabled, over a background picture.
Image {
    id: root

    /// Small text, for a picture-in-picture view.
    property bool useSmallFont: true

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    source:     "/res/NoVideoBackground.jpg"
    sourceSize: Qt.size(root.width, root.height)
    fillMode:   Image.PreserveAspectCrop

    Rectangle {
        anchors.centerIn:   parent
        width:              noVideoLabel.contentWidth + ScreenTools.defaultFontPixelHeight
        height:             noVideoLabel.contentHeight + ScreenTools.defaultFontPixelHeight
        radius:             ScreenTools.defaultFontPixelWidth / 2
        color:              Qt.rgba(0, 0, 0, 0.5)
    }

    QGCLabel {
        id:                 noVideoLabel
        anchors.centerIn:   parent
        text:               root._videoSettings.streamEnabled.rawValue ? qsTr("WAITING FOR VIDEO") : qsTr("VIDEO DISABLED")
        font.bold:          true
        color:              "white"
        font.pointSize:     root.useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
    }
}
