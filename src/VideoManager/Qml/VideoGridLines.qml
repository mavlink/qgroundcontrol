pragma ComponentBehavior: Bound

import QtQuick

// Rule-of-thirds grid lines over the video.
Item {
    id: root

    readonly property color _lineColor: Qt.rgba(1, 1, 1, 0.5)

    Repeater {
        model: 2

        Rectangle {
            required property int index

            x:      root.width * (index + 1) / 3
            width:  1
            height: root.height
            color:  root._lineColor
        }
    }

    Repeater {
        model: 2

        Rectangle {
            required property int index

            y:      root.height * (index + 1) / 3
            width:  root.width
            height: 1
            color:  root._lineColor
        }
    }
}
