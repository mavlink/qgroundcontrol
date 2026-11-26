pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// One video source in a tile: its picture over its name, or why there is no picture. A tap makes the source the main
// picture.
Rectangle {
    id: root

    /// The source shown: a stream or the USB camera.
    required property VideoStreamController source
    /// The manager the source is one of.
    property VideoManager videoManager: QGroundControl.videoManager
    /// Whether the tile is on display, as its row is: the source plays into it only then.
    property bool shown: true

    // A lost stream's last frame stays up, under a notice, while it reconnects.
    readonly property bool _streamLost: root.source.reconnecting
    readonly property bool _hasPicture: root.source.decoding || root._streamLost
    readonly property real _margin: ScreenTools.defaultFontPixelWidth / 2

    color:          qgcPal.mapWidgetBorderDark
    border.color:   qgcPal.mapWidgetBorderLight
    border.width:   1
    // The placeholder's picture is cropped to fill the tile, and a notice can be taller than a small one.
    clip:           true

    QGCPalette { id: qgcPal }

    NoVideoPlaceholder {
        anchors.fill:       parent
        anchors.margins:    root.border.width
        videoManager:       root.videoManager
        visible:            !root._hasPicture
    }

    // The source plays into its output: it has one only while the tile is on display, so the stream of a tile that
    // isn't shown isn't decoded. Not by the tile's visibility: out of sight behind another view, as the Fly View is
    // behind the Plan view, the stream plays on like the main picture's, or coming back would wait for a keyframe.
    Loader {
        anchors.fill:       parent
        anchors.margins:    root.border.width
        active:             root.shown
        visible:            root._hasPicture

        sourceComponent: VideoStreamOutput {
            stream: root.source
        }
    }

    // Frozen video must not pass for live.
    VideoStatusLabel {
        anchors.centerIn:   parent
        maximumWidth:       root.width - (2 * root._margin)
        text:               qsTr("VIDEO LOST - RECONNECTING")
        visible:            root._streamLost
    }

    Rectangle {
        height:             nameLabel.height + root._margin
        color:              qgcPal.mapWidgetBorderDark
        opacity:            0.6
        visible:            nameLabel.text !== ""

        anchors {
            left:       parent.left
            right:      parent.right
            bottom:     parent.bottom
            margins:    root.border.width
        }
    }

    QGCLabel {
        id:                     nameLabel
        text:                   root.source.displayName
        textFormat:             Text.PlainText
        color:                  qgcPal.mapWidgetBorderLight
        font.pointSize:         ScreenTools.smallFontPointSize
        horizontalAlignment:    Text.AlignHCenter
        elide:                  Text.ElideRight

        anchors {
            left:       parent.left
            right:      parent.right
            bottom:     parent.bottom
            margins:    root._margin
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      root.videoManager.selectSource(root.source)
    }
}
