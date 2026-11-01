pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

import QGroundControl
import QGroundControl.AnalyzeView
import QGroundControl.Controls

// Plays the videos QGroundControl recorded, newest first, with the telemetry of their subtitle files drawn over them.
AnalyzePage {
    id: root

    pageComponent:      recordingsComponent
    pageDescription:    qsTr("Play the videos QGroundControl recorded, with the telemetry recorded alongside them.")

    readonly property real _margin: ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    VideoRecordingList { id: recordingList }

    function _formatTime(ms) {
        const totalSeconds = Math.floor(ms / 1000)
        const seconds = totalSeconds % 60
        return Math.floor(totalSeconds / 60) + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function _formatSize(bytes) {
        return bytes >= 1024 * 1024 * 1024
                ? qsTr("%1 GB").arg((bytes / (1024 * 1024 * 1024)).toFixed(1))
                : qsTr("%1 MB").arg((bytes / (1024 * 1024)).toFixed(1))
    }

    Component {
        id: recordingsComponent

        RowLayout {
            id: page

            /// The path of the recording playing, empty for none.
            property string selectedPath: ""
            readonly property var _selected: {
                const recordings = recordingList.recordings
                for (let i = 0; i < recordings.length; i++) {
                    if (recordings[i].path === page.selectedPath) {
                        return recordings[i]
                    }
                }
                return null
            }
            readonly property bool _hasSubtitles: page._selected !== null && page._selected.subtitles.toString() !== ""

            width:      root.availableWidth
            height:     root.availableHeight
            spacing:    root._margin

            function _deleteSelected() {
                const path = page.selectedPath
                // Releases the file, which some platforms won't delete while it is open.
                page.selectedPath = ""
                if (!recordingList.remove(path)) {
                    QGroundControl.showMessageDialog(root, qsTr("Delete Recording"),
                                                     qsTr("The recording could not be deleted."))
                }
            }

            MediaPlayer {
                id: player

                source:         page._selected ? page._selected.url : ""
                videoOutput:    playerOutput
                audioOutput:    AudioOutput { }

                onSourceChanged: {
                    if (player.source.toString() !== "") {
                        player.play()
                    }
                }
            }

            SubtitleTrack {
                id: subtitles

                source:     page._hasSubtitles ? page._selected.subtitles : ""
                position:   player.position
            }

            QGCListView {
                id: recordingListView

                objectName:             "videoRecordingsList"
                model:                  recordingList.recordings
                spacing:                root._margin / 2
                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 32
                Layout.fillHeight:      true

                delegate: Rectangle {
                    id: recordingDelegate

                    required property var modelData

                    width:  recordingListView.width
                    height: recordingColumn.height + root._margin
                    color:  page.selectedPath === recordingDelegate.modelData.path ? qgcPal.accent : qgcPal.windowCard
                    radius: root._margin / 2

                    ColumnLayout {
                        id: recordingColumn

                        spacing: 0

                        anchors {
                            verticalCenter: parent.verticalCenter
                            left:           parent.left
                            right:          parent.right
                            margins:        root._margin / 2
                        }

                        QGCLabel {
                            text:               recordingDelegate.modelData.name
                            elide:              Text.ElideMiddle
                            color:              page.selectedPath === recordingDelegate.modelData.path ? qgcPal.accentText : qgcPal.textPrimary
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            text:               root._formatSize(recordingDelegate.modelData.size) + "  "
                                                + recordingDelegate.modelData.modified.toLocaleString(Qt.locale(), Locale.ShortFormat)
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              page.selectedPath === recordingDelegate.modelData.path ? qgcPal.accentText : qgcPal.textPrimary
                            Layout.fillWidth:   true
                        }
                    }

                    MouseArea {
                        anchors.fill:   parent
                        onClicked:      page.selectedPath = recordingDelegate.modelData.path
                    }
                }

                QGCLabel {
                    anchors.fill:           parent
                    anchors.margins:        root._margin
                    visible:                recordingListView.count === 0
                    text:                   qsTr("No recordings in %1").arg(recordingList.directory)
                    wrapMode:               Text.WrapAnywhere
                    horizontalAlignment:    Text.AlignHCenter
                }
            }

            ColumnLayout {
                spacing:            root._margin
                Layout.fillWidth:   true
                Layout.fillHeight:  true

                Rectangle {
                    color:              "black"
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true

                    VideoOutput {
                        id: playerOutput

                        anchors.fill:       parent
                        endOfStreamPolicy:  VideoOutput.KeepLastFrame
                    }

                    // The subtitle file's coordinates span the video's frame, as shown.
                    Item {
                        id: subtitleArea

                        readonly property real _scaleX: subtitleArea.width / subtitles.playResX
                        readonly property real _scaleY: subtitleArea.height / subtitles.playResY
                        // ASS font sizes are line heights, as players (libass) draw them, not em sizes.
                        readonly property real _fontPixelSize: Math.max(1, subtitles.fontSize * subtitleArea._scaleY
                                                                           * subtitleMetrics.font.pixelSize / subtitleMetrics.height)

                        x:          playerOutput.contentRect.x
                        y:          playerOutput.contentRect.y
                        width:      playerOutput.contentRect.width
                        height:     playerOutput.contentRect.height
                        visible:    page._hasSubtitles && telemetryCheckBox.checked

                        FontMetrics {
                            id: subtitleMetrics

                            font.family:    ScreenTools.fixedFontFamily
                            font.pixelSize: 100
                        }

                        Repeater {
                            model: subtitles.events

                            Text {
                                id: subtitleText

                                required property var modelData

                                // Alignment as a numeric keypad: the column and row of the text the anchor point is on.
                                readonly property int _column: (subtitleText.modelData.alignment - 1) % 3
                                readonly property int _row: Math.floor((subtitleText.modelData.alignment - 1) / 3)

                                x:                      subtitleText.modelData.x * subtitleArea._scaleX - subtitleText._column * subtitleText.width / 2
                                y:                      subtitleText.modelData.y * subtitleArea._scaleY - (2 - subtitleText._row) * subtitleText.height / 2
                                text:                   subtitleText.modelData.text
                                horizontalAlignment:    subtitleText._column === 0 ? Text.AlignLeft
                                                                                   : (subtitleText._column === 1 ? Text.AlignHCenter : Text.AlignRight)
                                color:                  "white"
                                style:                  Text.Outline
                                styleColor:             "black"
                                font.family:            ScreenTools.fixedFontFamily
                                font.pixelSize:         subtitleArea._fontPixelSize
                            }
                        }
                    }

                    QGCLabel {
                        anchors.centerIn:   parent
                        visible:            page.selectedPath === ""
                        text:               qsTr("Select a recording to play it")
                        color:              "white"
                    }

                    QGCLabel {
                        visible:    player.error !== MediaPlayer.NoError
                        text:       player.errorString
                        color:      "white"
                        wrapMode:   Text.WordWrap

                        anchors {
                            left:       parent.left
                            right:      parent.right
                            bottom:     parent.bottom
                            margins:    root._margin
                        }
                    }
                }

                RowLayout {
                    spacing:            root._margin
                    enabled:            page.selectedPath !== ""
                    Layout.fillWidth:   true

                    QGCButton {
                        objectName: "videoRecordingsPlayButton"
                        text:       player.playbackState === MediaPlayer.PlayingState ? qsTr("Pause") : qsTr("Play")
                        onClicked:  player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
                    }

                    QGCSlider {
                        id: seekSlider

                        from:               0
                        to:                 Math.max(1, player.duration)
                        value:              player.position
                        enabled:            player.seekable
                        Layout.fillWidth:   true
                        onMoved:            player.position = seekSlider.value
                    }

                    QGCLabel {
                        text: root._formatTime(player.position) + " / " + root._formatTime(player.duration)
                    }

                    QGCCheckBox {
                        id: telemetryCheckBox

                        text:       qsTr("Telemetry")
                        checked:    true
                        visible:    page._hasSubtitles
                    }

                    QGCButton {
                        text:       qsTr("Delete")
                        onClicked:  QGroundControl.showMessageDialog(
                                        root,
                                        qsTr("Delete Recording"),
                                        qsTr("%1 and its telemetry will be deleted permanently. Is this really what you want?")
                                            .arg(page._selected ? page._selected.name : ""),
                                        Dialog.Yes | Dialog.No,
                                        function() { page._deleteSelected() })
                    }
                }
            }
        }
    }
}
