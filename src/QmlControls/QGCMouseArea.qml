import QtQuick 2.3

import QGroundControl 1.0

MouseArea {
    Rectangle {
        anchors.fill:   parent
        border.color:   "red"
        border.width:   QGroundControl.showTouchAreas ? 1 : 0
        color:          "transparent"
    }
}
