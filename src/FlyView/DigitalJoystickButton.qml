import QtQuick
import QtQuick.Shapes 2.15

Shape {
    id: _root
    property real _t
    property color _buttonColor: "#FF0000"  
    property color _strokeColor: "#FFFFFF"
    property int   _strokeWidth: 0

    ShapePath {
        fillColor: _buttonColor         
        strokeWidth: _strokeWidth
        strokeColor: _strokeColor
        PathLine { x: 0;                    y: width; }
        PathLine { x: (_root._t/2) * width; y: (1 - (_root._t/2)) * width; }
        PathLine { x: (_root._t/2) * width; y: (_root._t/2) * width;}
        PathLine { x: 0;                    y: 0; }
    }

    ShapePath {
        
    }
}