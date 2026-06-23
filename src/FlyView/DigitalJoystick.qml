import QtQuick
import QtQuick.Shapes 2.15

Item {
    id: root
    signal buttonClicked(int id)

    property real _t: 0.5
    property color _joystickColor: "#FF0000"
    property int _strokeWidth: 3
    property color _strokeColor: "white"
    property int _hover: -1
    property int _strength: 10

    function getDegreeFromCoords(mouseX, mouseY) {
        let size = (height / 2);
        let dx = mouseX - size;
        let dy = mouseY - size;
        let angle = Math.atan2(dy, dx) * 180 / Math.PI;

        if (angle < 0) angle += 360;

        let id = idFromDegrees(angle);

        if(root._t != 1) {
            let innerDeadzone = size * (1 - root._t);
            if(Math.abs(dx) < innerDeadzone && Math.abs(dy) < innerDeadzone) {
                id += 4;
            }
        }
        return id;
    }

    function idFromDegrees(angle) {
        if (angle >= 135 && angle < 225) return 0;
        if (angle >= 45 && angle < 135) return 3;
        if (angle >= 225 && angle < 315) return 1;
        return 2;
    }

    Repeater {
        model: [0, 1, 2, 3] 

        DigitalJoystickButton {
            anchors.fill: parent
            _t: root._t
            rotation: modelData * 90
            _buttonColor: (modelData === root._hover) ? "#FF0000" : "black"
            _strokeWidth: root._strokeWidth
            _strokeColor: root._strokeColor
            _arrowSize: 0.7

        }
    }

    Repeater {
        model: [4, 5, 6, 7] 

        DigitalJoystickButton {
            width:  (1 - root._t) * root.height
            height: width
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
            _t: 1
            rotation: (modelData - 4) * 90
            _buttonColor: (modelData === root._hover) ? "#FF0000" : "black"
            _strokeWidth: 1
            _strokeColor: root._strokeColor
            _arrowSize: 0.3
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 999

        onPositionChanged: (mouse) => { root._hover = root.getDegreeFromCoords(mouse.x, mouse.y); }

        onExited: { 
            root._hover = -1; }

        onClicked: (mouse) => {
            root.buttonClicked(getDegreeFromCoords(mouse.x, mouse.y))

        }
    }

    
}

