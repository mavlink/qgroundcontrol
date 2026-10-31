pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.VideoManager

// Frame rate and bitrate of the measured video stream, orange while a stream degrades. The drawer shows each stream's
// statistics, and expands to the video settings that change a running stream.
Item {
    id:             control
    objectName:     "toolbar_videoStreamIndicator"
    width:          indicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _stats.valid

    readonly property VideoStreamController _primary: QGroundControl.videoManager.primaryStream
    readonly property VideoStreamController _thermal: QGroundControl.videoManager.thermalStream
    // The thermal stream's figures only while it is the one measured.
    readonly property var _stats: _primary.stats.valid ? _primary.stats : _thermal.stats
    readonly property bool _degraded: _streamDegraded(_primary) || _streamDegraded(_thermal)
    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    // Losing packets or dropping frames now, or showing under three quarters of the frame rate the camera advertises.
    function _streamDegraded(stream) {
        const stats = stream.stats
        const cameraFps = stream.streamInfo ? stream.streamInfo.framerate : 0
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
                objectName: "videoStreamBitrate"
                text:       VideoStreamFormat.bitrateText(control._stats.bitrateKbps)
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

                    QGCLabel {
                        text:       qsTr("No video stream")
                        visible:    !control._primary.stats.valid && !control._thermal.stats.valid
                    }
                }
            }

            expandedComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 4

                    QGCGroupBox {
                        heading: qsTr("Video Stream")
                        visible: QGroundControl.videoManager.isStreamSource

                        FactCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Low Latency Mode")
                            fact:               control._videoSettings.lowLatencyMode
                            visible:            fact.userVisible
                        }

                        LabelledFactTextField {
                            label:      qsTr("RTP jitter latency")
                            fact:       control._videoSettings.rtpJitterLatencyMs
                            visible:    fact.userVisible && !control._videoSettings.lowLatencyMode.rawValue
                        }
                    }

                    VideoDecodingSettingsGroup { }

                    VideoDisplaySettingsGroup { }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCButton {
                            text:       qsTr("Restart Video")
                            onClicked:  QGroundControl.videoManager.startVideo()
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
