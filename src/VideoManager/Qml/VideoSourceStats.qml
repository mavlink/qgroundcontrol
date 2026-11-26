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

        // Shown while its source is measured.
        VideoStreamStatsGroup {
            id: group

            required property VideoStreamController modelData

            // A stream in a tile goes by its name.
            heading:            group.modelData.role === VideoSourceRole.Camera ? qsTr("USB Camera")
                                : (group.modelData.tile ? group.modelData.displayName
                                   : (group.modelData.role === VideoSourceRole.Thermal ? qsTr("Thermal")
                                                                                       : qsTr("Video")))
            stream:             group.modelData
            Layout.fillWidth:   true
        }
    }

    QGCLabel {
        text:       qsTr("No video stream")
        visible:    !root.videoManager.measuredSource.stats.valid
    }
}
