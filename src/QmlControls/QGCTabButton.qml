import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

TabButton {
    id: control
    property bool _showHighlight: (pressed | hovered | checked)
    QGCPalette { id: qgcPalDisabled; colorGroupEnabled: false }
    background: Rectangle {
        color:                  enabled ? (_showHighlight ? qgcPal.buttonHighlight : qgcPal.button) : qgcPalDisabled.button
    }
    contentItem: QGCLabel {
        text:                   control.text
        color:                  enabled ? (_showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText) : qgcPalDisabled.buttonText
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        elide:                  Text.ElideRight
    }
}
