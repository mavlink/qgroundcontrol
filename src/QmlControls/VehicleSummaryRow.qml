import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelHeight

    property string labelText: "Label"
    property string valueText: "value"
    property string valueColor: ""

    QGCLabel {
        id: label
        Layout.fillWidth: true
        text: root.labelText
    }

    QGCLabel {
        Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 20
        text: root.valueText
        color: root.valueColor !== "" ? root.valueColor : QGroundControl.globalPalette.text
        elide: Text.ElideRight
    }
}
