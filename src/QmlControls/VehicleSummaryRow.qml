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
            color: root.valueColor !== "" ? root.valueColor : QGroundControl.globalPalette.text
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Qt.rgba(QGroundControl.globalPalette.text.r, QGroundControl.globalPalette.text.g, QGroundControl.globalPalette.text.b, 0.08)
        visible: root.showDivider
    }
}
