import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

TabButton {
    id: control
    property bool _showHighlight: (pressed | hovered | checked)
    background: Rectangle {
        color:                  _showHighlight ? qgcPal.buttonHighlight : qgcPal.button
    }
    contentItem: QGCLabel {
        text:                   control.text
        color:                  _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        elide:                  Text.ElideRight
    }
}
