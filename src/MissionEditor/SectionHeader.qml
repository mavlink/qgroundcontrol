import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Column {
    anchors.left:   parent.left
    anchors.right:  parent.right

    property alias text: label.text

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCLabel { id: label }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         1
        color:          qgcPal.text
    }
}
