pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Scrollable list of NTRIP caster mountpoints. Highlights the active selection
/// and emits mountpointSelected() when the user picks one. The model rows must
/// expose: index, mountpoint, details.
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
        required property string  details
        readonly property bool    selected: entry.mountpoint === root.selectedMountpoint

        width:      ListView.view.width
        height:     mountRow.height + ScreenTools.defaultFontPixelHeight * 0.5
        radius:     ScreenTools.defaultFontPixelHeight * 0.25
        color:      entry.selected ? qgcPal.buttonHighlight
                               : entry.index % 2 === 0 ? qgcPal.windowShade : qgcPal.window

        RowLayout {
            id:             mountRow
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: ScreenTools.defaultFontPixelWidth

            // The filling details label lets this column take the free space, keeping buttons right-aligned.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 0

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:       entry.mountpoint
                        textFormat: Text.PlainText
                        font.bold:  true
                        color:      entry.selected ? qgcPal.buttonHighlightText : qgcPal.text
                    }
                    QGCLabel {
                        visible:    entry.selected
                        text:       qsTr("(selected)")
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:      qgcPal.buttonHighlightText
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    elide: Text.ElideRight
                    textFormat: Text.PlainText
                    text: entry.details
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:  entry.selected ? qgcPal.buttonHighlightText : qgcPal.colorGrey
                }
            }

            QGCButton {
                text:       entry.selected ? qsTr("Selected") : qsTr("Select")
                enabled:    !entry.selected
                onClicked:  root.mountpointSelected(entry.mountpoint)
            }
        }
    }
}
