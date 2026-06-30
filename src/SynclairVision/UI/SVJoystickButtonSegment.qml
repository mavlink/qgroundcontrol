import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl

Item {
    id: root

    readonly property var startAngle: -Math.PI / 4
    readonly property var endAngle: Math.PI / 4

    property int radius: width / 2
    property color buttonColor
    property color hoveredButtonColor
    property color clickedButtonColor
    property color borderColor
    property color outerBorderColor
    property int   hoverIndex: -1

    property real   arrowSize  
    property real   arrowSpace

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
                antialiasing: true

                ShapePath {
                    strokeWidth: 1
                    strokeColor: borderColor

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

            Shape {
                id: arrow
                anchors.fill: parent
                property var radiusX: radius
                property var radiusY: radius

                property var minX: radiusX + ((1 - arrowSpace) * radiusX)
                property var maxX: width

                property var arrowSpaceWidth: maxX - minX

                property var spacing: (arrowSpaceWidth * (1 - arrowSize)) / 2
                property var heightY: (arrowSpaceWidth * arrowSize) / 1.5

                ShapePath {
                    strokeWidth: 0
                    fillColor: outerBorderColor

                    startX: arrow.maxX - arrow.spacing
                    startY: arrow.radiusY

                    PathLine {
                        x: arrow.minX + arrow.spacing
                        y: arrow.radiusY + arrow.heightY
                    }

                    PathLine {
                        x: arrow.minX + arrow.spacing
                        y: arrow.radiusY - arrow.heightY
                    }

                    PathLine {
                        x: arrow.maxX - arrow.spacing
                        y: arrow.radiusY
                    }
                }
            }
        }
    }

    Rectangle {
        id: outerBorder
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: outerBorderColor
        radius: width / 2
    }
}
