pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// The health of the video sources, one group for each that is measured, or a note that none is.
ColumnLayout {
    id: root

    /// The manager whose sources are shown.
    property VideoManager videoManager: QGroundControl.videoManager

    spacing: ScreenTools.defaultFontPixelHeight / 4

    Repeater {
        model: root.videoManager.sources

        Loader {
            id: groupLoader

            required property VideoStreamController modelData

            // The loader of a group that hides itself would still take a row.
            visible:            groupLoader.modelData.stats.valid
            sourceComponent:    groupLoader.modelData.role === VideoSourceRole.Camera ? cameraGroupComponent
                                                                                      : streamGroupComponent
            Layout.fillWidth:   true

            Component {
                id: streamGroupComponent

                VideoStreamStatsGroup {
                    // A stream in a tile goes by its name.
                    heading:    groupLoader.modelData.tile ? groupLoader.modelData.displayName
                                : (groupLoader.modelData.role === VideoSourceRole.Thermal ? qsTr("Thermal")
                                                                                          : qsTr("Video"))
                    stream:     groupLoader.modelData
                }
            }

            Component {
                id: cameraGroupComponent

                UVCStatsGroup {
                    camera:     root.videoManager.uvcCamera
                    cameraName: groupLoader.modelData.displayName
                }
            }
        }
    }

    QGCLabel {
        text:       qsTr("No video stream")
        visible:    !root.videoManager.measuredSource.stats.valid
    }
}
