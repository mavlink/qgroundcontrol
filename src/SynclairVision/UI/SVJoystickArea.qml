import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    property real t: 0.5
    property int hoverIndex: -1
    property int pressedIndex: -1

    property var  innerClicked: [false, false, false, false]
    property var  outerClicked: [false, false, false, false]

    QGCPalette { id: qgcPalette}

    function getAngleStep(dx, dy) {
        let angle = Math.atan2(dy, dx)
        if (angle < 0) angle += Math.PI * 2;
        angle /= (Math.PI / 4);
        return angle
    }

    function getButton(angleStep) {
        if(angleStep >= 5 && angleStep < 7) {
            return 3;
        } else if(angleStep >= 3 && angleStep < 5) {
            return 2;
        } else if(angleStep >= 1 && angleStep < 3) {
            return 1;
        } else {
            return 0;
        }
    }

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
        const angleStep = getAngleStep(dx, dy);
        const buttonIndex = offset + getButton(angleStep);
        return buttonIndex;
    }

    function setClicked(index) {
        if (index < 0) {
            return
        }

        if (index < 4) {
            let next = outerClicked.slice()
            next[index] = true
            outerClicked = next
        } else {
            let next = innerClicked.slice()
            next[index - 4] = true
            innerClicked = next
        }
    }

    

    function clearClicked(index) {
        if (index < 0) {
            return
        }

        if (index < 4) {
            let next = outerClicked.slice()
            next[index] = false
            outerClicked = next
        } else {
            let next = innerClicked.slice()
            next[index - 4] = false
            innerClicked = next
        }
    }

     

    SVJoystickButtonSegment {
        id: outerButtons
        anchors.fill: parent
        buttonColor: qgcPalette.windowShade
        hoveredButtonColor: qgcPalette.windowShadeLight
        clickedButtonColor: qgcPalette.buttonHighlight
        borderColor: qgcPalette.windowShadeLight
        outerBorderColor: qgcPalette.statusPassedText
        hoverIndex: (root.hoverIndex >= 0 && root.hoverIndex < 4) ? root.hoverIndex : -1
        clicked: outerClicked 
        arrowSize: 0.45  
        arrowSpace: 1 - root.t
    }

     SVJoystickButtonSegment {
        id: innerButtons
        width: parent.width * root.t
        height: parent.height * root.t
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        buttonColor: qgcPalette.window
        hoveredButtonColor: qgcPalette.windowShadeLight
        clickedButtonColor: qgcPalette.buttonHighlight
        borderColor: qgcPalette.windowShadeLight
        outerBorderColor: qgcPalette.statusPassedText
        hoverIndex: (root.hoverIndex >= 4) ? root.hoverIndex - 4 : -1

        clicked: innerClicked
        arrowSize: 0.3
        arrowSpace: 0.8
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 999

        onPositionChanged: (mouse) => {
            const index = root.getHoveredButton(mouse.x, mouse.y)
            root.hoverIndex = index

            if (pressed && index !== root.pressedIndex) {
                root.clearClicked(root.pressedIndex)
                root.setClicked(index)
                root.pressedIndex = index
            }
        }

        onExited: {
            root.hoverIndex = -1
            root.clearClicked(root.pressedIndex)
            root.pressedIndex = -1
        }

        onPressed: (mouse) => {
            const index = root.getHoveredButton(mouse.x, mouse.y)
            root.hoverIndex = index
            root.pressedIndex = index
            root.setClicked(index)
        }

        onReleased: {
            root.clearClicked(root.pressedIndex)
            root.pressedIndex = -1
        }

        onCanceled: {
            root.clearClicked(root.pressedIndex)
            root.pressedIndex = -1
            root.hoverIndex = -1
        }
    }
}
