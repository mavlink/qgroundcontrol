import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

// An IndicatorButton works just like q QGCButton with the additional support or a red/green
// indicator on the right edge.

QGCButton {
    property bool indicatorGreen: false

    Rectangle {
        readonly property real indicatorRadius: 4

        x: parent.width - (indicatorRadius * 2) - 5
        y: (parent.height - (indicatorRadius * 2)) / 2
        width: indicatorRadius * 2
        height: indicatorRadius * 2

        radius: indicatorRadius
        color: indicatorGreen ? "#00d932" : "red"
    }
}
