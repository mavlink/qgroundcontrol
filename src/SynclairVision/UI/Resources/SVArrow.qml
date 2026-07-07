import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls

    Shape {
        id: root
        property var arrowFilled: true
        property var outerBorderColor: "white"

        ShapePath {
            strokeWidth: (arrowFilled) ? 0 : 1
            strokeColor: outerBorderColor
            fillColor: (arrowFilled) ? outerBorderColor : 'white'
                    
            startX: width
            startY: height / 2
            PathLine { x: 0; y: height }      // bottom-left
            PathLine { x: 0; y: 0 }           // top-left
            PathLine { x: width; y: height / 2 }
        }
    }
