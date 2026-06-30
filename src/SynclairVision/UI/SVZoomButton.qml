import QtQuick
import QtQuick.Shapes 2.15
import QGroundControl

Item {
    id: root

    property color buttonColor
    property string buttonText
    property color textColor
    readonly property int radiusX: width / 2
    readonly property int radiusY: height / 2

    Shape {
        id: shape
        anchors.fill: parent
        antialiasing: true

        ShapePath {
            strokeWidth: 0
            fillColor: buttonColor
            startX: 0
            startY: 0

            PathLine {
                x: 0
                y: root.radiusY
            }

            PathArc {
                x: root.radiusX
                y: root.radiusY * 2
                radiusX: root.radiusX
                radiusY: root.radiusY
                direction: PathArc.Counterclockwise
            }

            PathArc {
                x: root.radiusX * 2
                y: root.radiusY
                radiusX: root.radiusX
                radiusY: root.radiusY
                direction: PathArc.Counterclockwise
            }

            PathLine {
                x: root.radiusX * 2
                y: 0
            }

            PathLine {
                x: 0
                y: 0
            }
        }

        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: - root.radiusY * 0.1
            text: buttonText
            font.pixelSize: radiusX * 1.5
            color: textColor
        }
    }
}