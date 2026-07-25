import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 0

    property string labelText: "Label"
    property string valueText: "value"
    property string valueColor: ""
    property bool showDivider: true

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    RowLayout {
        id: rowLayout
        Layout.fillWidth: true
        Layout.topMargin: ScreenTools.defaultFontPixelHeight * 0.2
        Layout.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.2
        spacing: ScreenTools.defaultFontPixelHeight

        QGCLabel {
            id: label
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            text: root.labelText
            wrapMode: Text.WordWrap
        }

        QGCLabel {
            id: valueLabel
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight
            text: root.valueText
            color: root.valueColor !== "" ? root.valueColor : qgcPal.text
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.08)
        visible: root.showDivider
    }
}
