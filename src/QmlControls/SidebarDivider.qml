import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Horizontal separator between groups in a sidebar navigation list
Item {
    Layout.fillWidth:       true
    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight / 2

    QGCPalette { id: qgcPal }

    Rectangle {
        anchors.left:           parent.left
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        height:                 1
        color:                  qgcPal.windowShade
    }
}
