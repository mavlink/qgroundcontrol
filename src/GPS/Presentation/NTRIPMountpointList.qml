import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Layouts

/// Scrollable list of NTRIP caster mountpoints. Highlights the active selection
/// and emits mountpointSelected() when the user picks one. The model rows must
/// expose: index, mountpoint, format, navSystem, country, bitrate, distanceKm.
QGCListView {
    id: root

    /// Mountpoint of the active selection (rendered as highlighted / "Selected").
    property string selectedMountpoint

    signal mountpointSelected(string mountpoint)

    Layout.fillWidth: true
    Layout.preferredHeight: Math.min(root.contentHeight, ScreenTools.defaultFontPixelHeight * 20)
    spacing: ScreenTools.defaultFontPixelHeight * 0.25

    delegate: Rectangle {
        id: entry

        required property int bitrate
        required property string country
        required property real distanceKm
        required property string format
        required property int index
        required property string mountpoint
        required property string navSystem

        color: {
            if (entry.mountpoint === root.selectedMountpoint)
                return qgcPal.buttonHighlight;
            return entry.index % 2 === 0 ? qgcPal.windowShade : qgcPal.window;
        }
        height: mountRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.5
        radius: ScreenTools.defaultFontPixelHeight * 0.25
        width: ListView.view.width

        RowLayout {
            id: mountRow

            anchors.left: parent.left
            anchors.margins: ScreenTools.defaultFontPixelWidth
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 0
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        color: entry.mountpoint === root.selectedMountpoint ? qgcPal.buttonHighlightText : qgcPal.text
                        elide: Text.ElideRight
                        font.bold: true
                        objectName: "mountpointName_" + entry.index
                        text: entry.mountpoint
                    }

                    QGCLabel {
                        color: qgcPal.buttonHighlightText
                        font.pointSize: ScreenTools.smallFontPointSize
                        text: qsTr("(selected)")
                        visible: entry.mountpoint === root.selectedMountpoint
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    color: entry.mountpoint === root.selectedMountpoint ? qgcPal.buttonHighlightText : qgcPal.colorGrey
                    elide: Text.ElideRight
                    font.pointSize: ScreenTools.smallFontPointSize
                    maximumLineCount: 2
                    objectName: "mountpointDescription_" + entry.index
                    text: {
                        var parts = [];
                        if (entry.format)
                            parts.push(entry.format);
                        if (entry.navSystem)
                            parts.push(entry.navSystem);
                        if (entry.country)
                            parts.push(entry.country);
                        if (entry.bitrate > 0)
                            parts.push(entry.bitrate + " bps");
                        if (entry.distanceKm >= 0)
                            parts.push(entry.distanceKm.toFixed(1) + " km");
                        return parts.join(" · ");
                    }
                    wrapMode: Text.Wrap
                }
            }

            QGCButton {
                Layout.minimumWidth: implicitWidth
                enabled: entry.mountpoint !== root.selectedMountpoint
                objectName: "mountpointSelect_" + entry.index
                text: entry.mountpoint === root.selectedMountpoint ? qsTr("Selected") : qsTr("Select")

                onClicked: root.mountpointSelected(entry.mountpoint)
            }
        }
    }

    QGCPalette {
        id: qgcPal
    }
}
