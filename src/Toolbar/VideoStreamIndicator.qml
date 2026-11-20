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

    readonly property VideoStreamController _measuredSource: root.videoManager.measuredSource
    readonly property videoStreamStats _stats: _measuredSource.stats

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
            }

            QGCLabel {
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
