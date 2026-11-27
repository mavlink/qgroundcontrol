pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

// Frame rate and bitrate of the measured video source (its resolution where the receiver can't count the bitrate, as
// for the USB camera), orange while a source degrades. While no source is measured, what keeps the video from playing.
// Shown while video is wanted, playing or not: the drawer, which shows each source's statistics and expands to the
// video settings that change a running stream, has the only "Restart Video".
Item {
    id: root

    property bool showIndicator: _stats.valid || _videoWanted
    /// The manager whose sources the indicator reports.
    property VideoManager videoManager: QGroundControl.videoManager

    readonly property VideoStreamController _measuredSource: root.videoManager.measuredSource
    readonly property videoStreamStats _stats: _measuredSource.stats
    readonly property VideoStreamController _primaryStream: root.videoManager.primaryStream
    readonly property int _noVideoReason: root.videoManager.noVideoReason
    // A source is enabled and configured. "Disable when disarmed" takes hasVideo away while it holds, and the stream
    // it keeps from playing is wanted all the same: "Restart Video" plays it.
    readonly property bool _videoWanted: root.videoManager.hasVideo
                                         || (root._noVideoReason === VideoManager.NoVideoReason.DisabledWhileDisarmed
                                             && root._primaryStream.uri !== "")
    // As the video view tells them apart, in fewer words.
    readonly property string _stateText: {
        switch (root._noVideoReason) {
        case VideoManager.NoVideoReason.DisabledWhileDisarmed:
            return qsTr("Disabled while disarmed")
        case VideoManager.NoVideoReason.Stopped:
            return qsTr("Stopped")
        default:
            return root.videoManager.mainSource.reconnecting ? qsTr("Reconnecting") : qsTr("Waiting")
        }
    }

    objectName:     "toolbar_videoStreamIndicator"
    width:          indicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    QGCPalette { id: qgcPal }

    Row {
        id:             indicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            objectName:         "videoStreamIcon"
            source:             "/InstrumentValueIcons/video-camera.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            color:              root.videoManager.degraded ? qgcPal.colorOrange : qgcPal.textPrimary
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                text:       qsTr("%1 fps").arg(root._stats.fps.toFixed(0))
                visible:    root._stats.valid
            }

            QGCLabel {
                text:       root._stats.bitrateKbps >= 0 ? VideoStreamFormat.bitrateText(root._stats.bitrateKbps)
                                                         : qsTr("%1p").arg(root._measuredSource.videoSize.height)
                visible:    root._stats.valid
            }

            // In their place, on two lines at most.
            QGCLabel {
                objectName: "videoStreamState"
                width:      Math.min(implicitWidth, ScreenTools.defaultFontPixelWidth * 14)
                text:       root._stateText
                visible:    !root._stats.valid
                wrapMode:   Text.WordWrap
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(videoStreamPage, root)
    }

    Component {
        id: videoStreamPage

        ToolIndicatorPage {
            showExpand: true

            contentComponent: Component {
                VideoSourceStats {
                    videoManager: root.videoManager
                }
            }

            expandedComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 4

                    VideoStreamSettingsGroup { }

                    VideoDecodingSettingsGroup { }

                    VideoDisplaySettingsGroup { }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCButton {
                            objectName: "videoStreamRestartButton"
                            text:       qsTr("Restart Video")
                            visible:    root.videoManager.isStreamSource
                            onClicked:  root.videoManager.startVideo()
                        }

                        QGCButton {
                            text:       qsTr("All Video Settings")
                            onClicked: {
                                if (mainWindow.allowViewSwitch()) {
                                    mainWindow.closeIndicatorDrawer()
                                    mainWindow.showSettingsTool("Video")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
