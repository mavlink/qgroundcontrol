pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

// Frame rate and bitrate of the measured video source (its resolution where the receiver can't count the bitrate, as
// for the USB camera), orange while a source degrades. The drawer shows each source's statistics, and expands to the
// video settings that change a running stream.
Item {
    id: root

    property bool showIndicator: _stats.valid
    /// The manager whose sources the indicator reports.
    property VideoManager videoManager: QGroundControl.videoManager

    // The first source that is measured, which puts the primary stream before the thermal stream; the USB camera runs
    // in the primary stream's place. A source in a tile comes after those of the main picture.
    readonly property VideoStreamController _measuredSource: {
        const sources = root.videoManager.sources
        let inTile = null
        for (let i = 0; i < sources.length; i++) {
            if (sources[i].stats.valid) {
                if (!sources[i].tile) {
                    return sources[i]
                }
                if (!inTile) {
                    inTile = sources[i]
                }
            }
        }
        return inTile ? inTile : sources[0]
    }
    readonly property videoStreamStats _stats: _measuredSource.stats
    readonly property bool _degraded: {
        const sources = root.videoManager.sources
        for (let i = 0; i < sources.length; i++) {
            if (root._statsDegraded(sources[i].stats, root._cameraFps(sources[i]))) {
                return true
            }
        }
        return false
    }

    // The frame rate the source's camera is set up for: the USB camera's own, or the one a stream's camera advertises.
    // None for a stream that isn't decoded, as the thermal stream while its view is off, which shows no frames to
    // compare with it.
    function _cameraFps(source) {
        if (source.role === VideoSourceRole.Camera) {
            return root.videoManager.uvcCamera.monitor.cameraFrameRate
        }
        return source.decoding && source.streamInfo ? source.streamInfo.framerate : 0
    }

    // Losing packets or dropping frames now, or showing under three quarters of the frame rate the camera is set up for.
    function _statsDegraded(stats, cameraFps) {
        return stats.valid && (stats.lossPercent > 0 || stats.droppedFps > 0
                               || (cameraFps > 0 && stats.fps < cameraFps * 0.75))
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
            color:              root._degraded ? qgcPal.colorOrange : qgcPal.textPrimary
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                objectName: "videoStreamFrameRate"
                text:       qsTr("%1 fps").arg(root._stats.fps.toFixed(0))
            }

            QGCLabel {
                objectName: "videoStreamDetail"
                text:       root._stats.bitrateKbps >= 0 ? VideoStreamFormat.bitrateText(root._stats.bitrateKbps)
                                                         : qsTr("%1p").arg(root._measuredSource.videoSize.height)
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
                            text:       qsTr("Restart Video")
                            visible:    !root.videoManager.isUvc
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
