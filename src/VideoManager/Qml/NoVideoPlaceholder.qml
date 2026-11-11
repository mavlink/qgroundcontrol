import QtQuick

import QGroundControl
import QGroundControl.Controls

// What a video view shows without video: why it has none, over a background picture.
Image {
    id: root

    /// Small text, for a picture-in-picture view.
    property bool useSmallFont: true
    /// The manager whose reason for having no video is shown.
    property VideoManager videoManager: QGroundControl.videoManager

    readonly property string _reasonText: {
        switch (root.videoManager.noVideoReason) {
        case VideoManager.NoVideoReason.DisabledWhileDisarmed:
            return qsTr("VIDEO DISABLED WHILE DISARMED")
        case VideoManager.NoVideoReason.Stopped:
            return qsTr("VIDEO STOPPED")
        default:
            return qsTr("WAITING FOR VIDEO")
        }
    }

    source:     "/res/NoVideoBackground.jpg"
    sourceSize: Qt.size(root.width, root.height)
    fillMode:   Image.PreserveAspectCrop

    VideoStatusLabel {
        anchors.centerIn:   parent
        useSmallFont:       root.useSmallFont
        maximumWidth:       root.width
        text:               root._reasonText
    }
}
