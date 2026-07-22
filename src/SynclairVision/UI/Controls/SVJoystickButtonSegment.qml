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
    property color borderColor: qgcPalette.windowShadeLight
    property color arrowColor: "white"
    property color hoverGlowColor: buttonColor
    property int   hoverIndex: -1
    property point hoverPosition: Qt.point(width / 2, height / 2)

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
            z: index === root.hoverIndex ? 1 : 0
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
                    strokeColor: (clicked[index] || index === hoverIndex)  ? "white" : qgcPalette.windowShadeLight

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

            Canvas {
                id: hoverGlow
                opacity: 0.5

                anchors.fill: parent
                visible: index === root.hoverIndex

                property point glowPosition: button.mapFromItem(root, root.hoverPosition.x, root.hoverPosition.y)
                property bool segmentClicked: root.clicked[index]

                onGlowPositionChanged: {
                    if (visible) {
                        requestPaint()
                    }
                }

                onSegmentClickedChanged: {
                    if (visible) {
                        requestPaint()
                    }
                }

                onPaint: {
                    if (index !== root.hoverIndex) {
                        return
                    }

                    const context = getContext("2d")
                    const centerX = width / 2
                    const centerY = height / 2
                    const glowRadius = Math.max(width, height) * 0.9
                    const glowColor =  Qt.rgba(1, 1, 1, 0.5)

                    context.reset()
                    context.clearRect(0, 0, width, height)
                    context.beginPath()
                    context.moveTo(centerX, centerY)
                    context.lineTo(button.px(root.startAngle), button.py(root.startAngle))
                    context.arc(centerX, centerY, root.radius, root.startAngle, root.endAngle)
                    context.closePath()
                    context.clip()

                    const gradient = context.createRadialGradient(glowPosition.x, glowPosition.y, 0, glowPosition.x, glowPosition.y, glowRadius)
                    gradient.addColorStop(0, Qt.rgba(glowColor.r, glowColor.g, glowColor.b, 0.36))
                    gradient.addColorStop(0.35, Qt.rgba(glowColor.r, glowColor.g, glowColor.b, 0.20))
                    gradient.addColorStop(0.75, Qt.rgba(glowColor.r, glowColor.g, glowColor.b, 0.05))
                    gradient.addColorStop(1, Qt.rgba(glowColor.r, glowColor.g, glowColor.b, 0.0))
                    context.fillStyle = gradient
                    context.fillRect(0, 0, width, height)
                }

                onVisibleChanged: {
                    if (visible) {
                        requestPaint()
                    }
                }

                onWidthChanged: {
                    if (visible) {
                        requestPaint()
                    }
                }

                onHeightChanged: {
                    if (visible) {
                        requestPaint()
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
        z: 2
    }
}
