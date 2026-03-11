import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id: root

    property string labelText: "Label"
    property string valueText: "value"
    property string valueColor: ""

    width: parent.width

    QGCLabel {
        id:     label
        text:   root.labelText
    }
    QGCLabel {
        text:                   root.valueText
        color:                  root.valueColor !== "" ? root.valueColor : QGroundControl.globalPalette.text
        elide:                  Text.ElideRight
        horizontalAlignment:    Text.AlignRight
        Layout.fillWidth:       true
    }
}
