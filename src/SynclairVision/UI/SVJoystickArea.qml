import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    property var _t: 0.5
    property var _hoveredButton: -1
    property bool _pressed: false

    QGCPalette { id: qgcPalette}

    function getHoveredButton(mouseX, mouseY) {
        const radius = root.width / 2;
        const innerRadius = radius / 2;

        const dx = mouseX - radius;
        const dy = mouseY - radius;
        const distance = Math.hypot(dx, dy);

        if(distance > radius) {
            return -1
        }

        const offset = distance < innerRadius ? 4 : 0;

        let angle = Math.atan2(dy, dx)
        if (angle < 0) angle += Math.PI * 2;
        const angleStep = Math.PI / 4

        if(angle >= angleStep * 5 && angle < angleStep * 7) {
            return offset + 3;
        } else if(angle >= angleStep * 3 && angle < angleStep * 5) {
            return offset + 2;
        } else if(angle >= angleStep && angle < angleStep * 3) {
            return offset + 1
        } else {
            return offset;
        }
    }

    SVJoystickButtonSegment {
        id: outerButtons
        anchors.fill: parent
        _buttonColor: qgcPalette.windowShade
        _buttonHoveredColor: (_pressed) ? qgcPalette.buttonHighlight : qgcPalette.primaryButton
        _borderColor: qgcPalette.windowShadeLight
        _outerBorderColor: qgcPalette.text
        _hover: _hoveredButton
    }

    SVJoystickButtonSegment {
        id: innerButtons
        width: root.width * 0.5
        height: root.height * 0.5
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        _buttonColor: qgcPalette.window
        _buttonHoveredColor: (_pressed) ? qgcPalette.buttonHighlight : qgcPalette.primaryButton
        _borderColor: qgcPalette.windowShadeLight
        _outerBorderColor: qgcPalette.windowShadeLight
        _hover: _hoveredButton - 4
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 999

        onPositionChanged: (mouse) => {
            root._hoveredButton = root.getHoveredButton(mouse.x, mouse.y)
        }

        onExited: { 
            root._hoveredButton = -1; 
            _pressed = false
        }

        onPressed: {
            _pressed = true
        }

        onReleased: {
            _pressed = false
        }
        onCanceled: {
    _       pressed = false
        }   




    }
}