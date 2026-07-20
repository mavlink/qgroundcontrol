import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl

Item {
    id: root

    readonly property var startAngle: -Math.PI / 4
    readonly property var endAngle: Math.PI / 4

    property real radius: width / 2
    property color buttonColor
    property color hoveredButtonColor
    property color clickedButtonColor
    property color borderColor
    property color arrowColor: "white"
    property int   hoverIndex: -1

    property bool  arrowFilled: true
    property real  arrowSize
    property real  arrowSpace
    property real  arrowOpacity: 1

    property real   arrowSpaceWidth: radius - (1 - arrowSpace) * radius
    property real   spacing: (arrowSpaceWidth * (1 - arrowSize)) / 2

    property var   clicked: new Array(4).fill(false)



    Repeater {
        id: innerButtons
        model: 4

        delegate: Item {
            id: button
            required property int index
            rotation: index * 90
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 8


            function px(angle) { return (width / 2) + (Math.cos(angle) * radius); }

            function py(angle) { return (height / 2) + (Math.sin(angle) * radius); }

            Shape {
                id: shape
                anchors.fill: parent
                //antialiasing: true

                ShapePath {
                    strokeWidth: 0

                    fillColor: {
                        if(clicked[index]) {
                            return clickedButtonColor
                        } else if (index === hoverIndex) {
                            return hoveredButtonColor
                        } else {
                            return buttonColor
                        }
                    }
                    
                    startX: width / 2
                    startY: height / 2

                    PathLine {
                        x: px(root.startAngle)
                        y: py(root.startAngle)
                    }

                    PathArc {
                        x: px(root.endAngle)
                        y: py(root.endAngle)
                        radiusX: radius
                        radiusY: radius
                    }

                    PathLine {
                        x: width / 2
                        y: height / 2
                    }

                }
            }

            SVArrow {
                id: arrow
                width: root.arrowSpaceWidth * root.arrowSize
                height: width * 1.3
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: root.spacing

                arrowFilled: root.arrowFilled
                outerBorderColor: root.arrowColor
                opacity: root.arrowOpacity
            }
        }
    }

    Rectangle {
        id: outerBorder
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: borderColor
        radius: width / 2
    }
}
