import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Scrollable list of NTRIP caster mountpoints. Highlights the active selection
/// and emits mountpointSelected() when the user picks one. The model rows must
/// expose: index, mountpoint, format, navSystem, country, bitrate, distanceKm.
QGCListView {
    id: root

    /// Mountpoint of the active selection (rendered as highlighted / "Selected").
    property string selectedMountpoint

    signal mountpointSelected(string mountpoint)

    Layout.fillWidth:       true
    Layout.preferredHeight: Math.min(root.contentHeight, ScreenTools.defaultFontPixelHeight * 20)
    spacing:                ScreenTools.defaultFontPixelHeight * 0.25

    QGCPalette { id: qgcPal }

    delegate: Rectangle {
        id: entry

        required property int     index
        required property string  mountpoint
        required property string  format
        required property string  navSystem
        required property string  country
        required property int     bitrate
        required property real    distanceKm

        width:      ListView.view.width
        height:     mountRow.height + ScreenTools.defaultFontPixelHeight * 0.5
        radius:     ScreenTools.defaultFontPixelHeight * 0.25
        color: {
            if (entry.mountpoint === root.selectedMountpoint)
                return qgcPal.buttonHighlight
            return entry.index % 2 === 0 ? qgcPal.windowShade : qgcPal.window
        }

        RowLayout {
            id:             mountRow
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: ScreenTools.defaultFontPixelWidth

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:       entry.mountpoint
                        font.bold:  true
                        color:      entry.mountpoint === root.selectedMountpoint
                                        ? qgcPal.buttonHighlightText : qgcPal.text
                    }
                    QGCLabel {
                        visible:    entry.mountpoint === root.selectedMountpoint
                        text:       qsTr("(selected)")
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:      qgcPal.buttonHighlightText
                    }
                }

                QGCLabel {
                    text: {
                        var parts = []
                        if (entry.format) parts.push(entry.format)
                        if (entry.navSystem) parts.push(entry.navSystem)
                        if (entry.country) parts.push(entry.country)
                        if (entry.bitrate > 0) parts.push(entry.bitrate + " bps")
                        if (entry.distanceKm >= 0) parts.push(entry.distanceKm.toFixed(1) + " km")
                        return parts.join(" · ")
                    }
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:  entry.mountpoint === root.selectedMountpoint
                                ? qgcPal.buttonHighlightText : qgcPal.colorGrey
                }
            }

            QGCButton {
                text:       entry.mountpoint === root.selectedMountpoint
                                ? qsTr("Selected") : qsTr("Select")
                enabled:    entry.mountpoint !== root.selectedMountpoint
                onClicked:  root.mountpointSelected(entry.mountpoint)
            }
        }
    }
}
