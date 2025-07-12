import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    property string labelText: "Label"
    property string valueText: "value"

    width: parent.width

    QGCLabel {
        id:     label
        text:   labelText
    }
    QGCLabel {
        text:                   valueText
        elide:                  Text.ElideRight
        horizontalAlignment:    Text.AlignRight
        Layout.fillWidth:       true
    }
}
