import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

// An IndicatorButton works just like q QGCButton with the additional support or a red/green
// indicator on the right edge.

QGCButton {
    property bool indicatorGreen: false

    Rectangle {
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 3
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        width:                  radius * 2
        height:                 width
        radius:                 (ScreenTools.defaultFontPixelHeight * .75) / 2
        color:                  indicatorGreen ? "#00d932" : "red"
    }
}
