import QtQuick
import QtQuick.Shapes 2.15

Item {
    id: _root
    property real _t
    property color _buttonColor: "#FF0000"  
    property color _strokeColor: "#FFFFFF"
    property int   _strokeWidth: 0
    property var   _arrowSize: 0.5

    property real cx: (_t !== 1) ? (_root._t / 4) * _root.width : (_root._t / 6) * _root.width    // halfway to the inner corner
    property real cy: _root.width / 2                 // vertical center
    property real r:  (_root._arrowSize * (_root._t / 2) * _root.width) / 2  // scales with wedge size

    Shape {
        id: button
        anchors.fill: parent

        ShapePath {
            fillColor: _buttonColor         
            strokeWidth: _strokeWidth
            strokeColor: _strokeColor
            PathLine { x: 0;                    y: width; }
            PathLine { x: (_root._t/2) * width; y: (1 - (_root._t/2)) * width; }
            PathLine { x: (_root._t/2) * width; y: (_root._t/2) * width;}
            PathLine { x: 0;                    y: 0; }
        }
    }

    Shape {
    id: arrow
    anchors.fill: parent
    ShapePath {
        fillColor:   _strokeColor
        strokeWidth: 0

        // Center of the wedge area
        

        startX: _root.cx - _root.r;  startY: _root.cy          // left point (base)
        PathLine { x: _root.cx + _root.r; y: _root.cy - _root.r }    // top-right
        PathLine { x: _root.cx + _root.r; y: _root.cy + _root.r }    // bottom-right
        PathLine { x: _root.cx - _root.r; y: _root.cy     }    // back to base
    }
}
}

