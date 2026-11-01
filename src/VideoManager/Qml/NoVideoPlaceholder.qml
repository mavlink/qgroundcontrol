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

    VideoStatusLabel {
        anchors.centerIn:   parent
        useSmallFont:       root.useSmallFont
        text:               root._videoSettings.streamEnabled.rawValue ? qsTr("WAITING FOR VIDEO") : qsTr("VIDEO DISABLED")
    }
}
