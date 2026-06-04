import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Messages tab for the Log Viewer.
ScrollView {
    id: control

    required property var logParser

    clip: true

    QGCPalette { id: qgcPal }

    ListView {
        anchors.fill: parent
        model: logParser.messages
        spacing: ScreenTools.defaultFontPixelHeight * 0.2
        clip: true
        ScrollBar.vertical: ScrollBar { }

        delegate: Rectangle {
            width: ListView.view.width
            height: _msgRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.4
            color: index % 2 === 0 ? qgcPal.windowShade : qgcPal.windowShadeDark
            radius: 2

            RowLayout {
                id: _msgRow
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                QGCLabel {
                    readonly property double _t: Number(modelData.time)
                    text: (isNaN(_t) || _t < 0) ? "" : _t.toFixed(3) + "s"
                    color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.6)
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 10
                    horizontalAlignment: Text.AlignRight
                }

                QGCLabel {
                    text: String(modelData.text)
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                }
            }
        }
    }
}
