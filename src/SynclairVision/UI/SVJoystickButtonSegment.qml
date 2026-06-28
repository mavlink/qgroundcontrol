import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl

Item {
    id: root

    property var _radius: width / 2
    property color _buttonColor
    property color _buttonHoveredColor
    property color _borderColor
    property color _outerBorderColor
    property var   _hover

    readonly property var startAngle: -Math.PI / 4
    readonly property var endAngle: Math.PI / 4

    Repeater {
        id: innerButtons
        model: 4

        delegate: Item {
            id: button
            required property int index
            rotation: index * 90
            anchors.fill: parent

            function px(angle) { return _radius + (Math.cos(angle) * _radius); }

            function py(angle) { return _radius + (Math.sin(angle) * _radius); }

            Shape {
                id: shape
                anchors.fill: parent
                antialiasing: true

                ShapePath {
                    strokeWidth: 1
                    strokeColor: _borderColor
                    fillColor: (index === _hover) ? _buttonHoveredColor : _buttonColor 
                    
                    startX: _radius
                    startY: _radius

                    PathLine {
                        x: px(root.startAngle)
                        y: py(root.startAngle)
                    }

                    PathArc {
                        x: px(root.endAngle)
                        y: py(root.endAngle)
                        radiusX: _radius
                        radiusY: _radius
                    }

                    PathLine {
                        x: _radius
                        y: _radius
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
        border.color: _outerBorderColor
        radius: width / 2
    }
}