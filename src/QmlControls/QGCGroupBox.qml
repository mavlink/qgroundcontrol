import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

GroupBox {
    id: control

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    background: Rectangle {
        y:      control.topPadding - control.padding
        width:  parent.width
        height: parent.height - control.topPadding + control.padding
        color:  qgcPal.windowShade
    }

    label: QGCLabel {
        width:  control.availableWidth
        text:   control.title
    }
}
