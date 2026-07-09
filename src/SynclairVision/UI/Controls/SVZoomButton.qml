import QtQuick
import QtQuick.Shapes 2.15
import QGroundControl

Item {
    id: root

    property color buttonColor
    property string text
    property color textColor

    readonly property real halfWidth: width / 2
    readonly property real halfHeight: height / 2

    Shape {
        anchors.fill: parent
        antialiasing: true

        ShapePath {
            strokeWidth: 0
            fillColor: root.buttonColor
            startX: 0
            startY: 0

            PathLine { x: 0; y: root.halfHeight }

            PathArc {
                x: root.halfWidth
                y: root.height
                radiusX: root.halfWidth
                radiusY: root.halfHeight
                direction: PathArc.Counterclockwise
            }

            PathArc {
                x: root.width
                y: root.halfHeight
                radiusX: root.halfWidth
                radiusY: root.halfHeight
                direction: PathArc.Counterclockwise
            }

            PathLine { x: root.width; y: 0 }
            PathLine { x: 0; y: 0 }
        }
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -SVUnits.smallMargin
        text: root.text
        font.pixelSize: root.halfWidth * 1.5
        color: root.textColor
    }
}