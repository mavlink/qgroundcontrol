pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

// Frame rate and bitrate of the measured video stream, or frame rate and resolution of the USB camera, orange while a
// stream degrades. The drawer shows each stream's statistics, and expands to the video settings that change a running
// stream.
Item {
    id:             control
    objectName:     "toolbar_videoStreamIndicator"
    width:          indicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _stats.valid

    readonly property VideoManager          _videoManager:  QGroundControl.videoManager
    readonly property VideoStreamController _primary:       _videoManager.primaryStream
    readonly property VideoStreamController _thermal:       _videoManager.thermalStream
    readonly property UVCStreamMonitor      _uvc:           _videoManager.uvcMonitor
    readonly property bool _uvcMeasured: _uvc.stats.valid && !_primary.stats.valid && !_thermal.stats.valid
    // The thermal stream's figures only while it is the one measured.
    readonly property var _stats: _uvcMeasured ? _uvc.stats : (_primary.stats.valid ? _primary.stats : _thermal.stats)
    readonly property bool _degraded: _statsDegraded(_primary.stats, _streamCameraFps(_primary))
                                      || _statsDegraded(_thermal.stats, _streamCameraFps(_thermal))
                                      || _statsDegraded(_uvc.stats, _uvc.cameraFrameRate)

    function _streamCameraFps(stream) {
        return stream.streamInfo ? stream.streamInfo.framerate : 0
    }

    // Losing packets or dropping frames now, or showing under three quarters of the frame rate the camera advertises.
    function _statsDegraded(stats, cameraFps) {
        return stats.valid && (stats.lossPercent > 0 || stats.droppedFps > 0
                               || (cameraFps > 0 && stats.fps < cameraFps * 0.75))
    }

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
            color:              control._degraded ? qgcPal.colorOrange : qgcPal.textPrimary
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                objectName: "videoStreamFrameRate"
                text:       qsTr("%1 fps").arg(control._stats.fps.toFixed(0))
            }

            QGCLabel {
                objectName: "videoStreamDetail"
                text:       control._uvcMeasured ? qsTr("%1p").arg(control._uvc.videoSize.height)
                                                 : VideoStreamFormat.bitrateText(control._stats.bitrateKbps)
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(videoStreamPage, control)
    }

    Component {
        id: videoStreamPage

        ToolIndicatorPage {
            showExpand: true

            contentComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 4

                    VideoStreamStatsGroup {
                        heading:    qsTr("Video")
                        stream:     control._primary
                    }

                    VideoStreamStatsGroup {
                        heading:    qsTr("Thermal")
                        stream:     control._thermal
                    }

                    UVCStatsGroup {
                        monitor: control._uvc
                    }

                    QGCLabel {
                        text:       qsTr("No video stream")
                        visible:    !control._stats.valid
                    }
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
                            visible:    !control._videoManager.isUvc
                            onClicked:  control._videoManager.startVideo()
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
