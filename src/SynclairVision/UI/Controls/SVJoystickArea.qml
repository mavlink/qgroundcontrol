import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls


Item {
    id: root

    property real t: SVSettings.joystickRatio
    property int hoverIndex: -1
    property int pressedIndex: -1
    readonly property point hoverPosition: Qt.point(mouseArea.mouseX, mouseArea.mouseY)

    property var  innerClicked: [false, false, false, false]
    property var  outerClicked: [false, false, false, false]
    property var  hasInnerRing: true
    readonly property var outerVisualClicked: [
        outerClicked[0] || (!SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[0]),
        outerClicked[1] || (!SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[1]),
        outerClicked[2] || (!SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[2]),
        outerClicked[3] || (!SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[3])
    ]
    readonly property var innerVisualClicked: [
        innerClicked[0] || (SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[0]),
        innerClicked[1] || (SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[1]),
        innerClicked[2] || (SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[2]),
        innerClicked[3] || (SVState.shortcutSmallMovementHeld && SVState.shortcutJoystickHeld[3])
    ]

    readonly property bool controlsLocked: SVState.lockControls
    readonly property bool controlsUsable: !SVState.lockControls && SVState.cameraSelected !== -1
    readonly property color arrowColor: root.controlsUsable ? "white" : qgcPalette.windowShadeLight
    readonly property real arrowOpacity: root.controlsUsable ? 1 : 0.45
    readonly property color borderColor: !root.controlsUsable ? qgcPalette.windowShadeLight : qgcPalette.statusPassedText

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

        const offset = ((distance < innerRadius) && (hasInnerRing)) ? 4 : 0;
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
        buttonColor: SVSettings.simplifiedUserInterface ? qgcPalette.windowShade : qgcPalette.windowTransparent
        hoveredButtonColor: SVSettings.simplifiedUserInterface ? qgcPalette.windowShadeLight : Qt.alpha(qgcPalette.windowShadeLight, 0.8) 
        clickedButtonColor: qgcPalette.buttonHighlight
        borderColor: root.borderColor
        hoverIndex: (root.hoverIndex >= 0 && root.hoverIndex < 4) ? root.hoverIndex : -1
        hoverPosition: outerButtons.mapFromItem(root, root.hoverPosition.x, root.hoverPosition.y)
        clicked: root.outerVisualClicked

        arrowFilled: (qgcPalette.globalTheme === QGCPalette.Light) ? false : true
        arrowSize: (root.hasInnerRing) ? 0.45 : 0.3
        arrowSpace: (root.hasInnerRing) ? 1 - root.t : 0.8
        arrowColor: root.arrowColor
        arrowOpacity: root.arrowOpacity
    }

    SVJoystickButtonSegment {
        id: innerButtons
        width: parent.width * root.t
        height: parent.height * root.t
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        buttonColor: SVSettings.simplifiedUserInterface ? qgcPalette.window : qgcPalette.windowTransparent
        hoveredButtonColor: SVSettings.simplifiedUserInterface ? qgcPalette.windowShadeLight : Qt.alpha(qgcPalette.windowShadeLight, 0.8) 
        clickedButtonColor: qgcPalette.buttonHighlight
        borderColor: root.borderColor
        hoverIndex: (root.hoverIndex >= 4) ? root.hoverIndex - 4 : -1
        hoverPosition: innerButtons.mapFromItem(root, root.hoverPosition.x, root.hoverPosition.y)

        clicked: root.innerVisualClicked
        arrowSize: 0.3
        arrowSpace: 0.8
        arrowColor: root.arrowColor
        arrowOpacity: root.arrowOpacity

        visible: hasInnerRing
    }

    Rectangle {
        width: parent.width - 2
        color: qgcPalette.windowShadeLight
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        height: 1
        rotation: 45
        visible: false
    }

    Rectangle {
        width: parent.width - 2
        color: qgcPalette.windowShadeLight
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        height: 1
        rotation: -45
        visible: false
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        z: 999

        onPositionChanged: (mouse) => {
            const index = root.getHoveredButton(mouse.x, mouse.y)

            if (!root.controlsUsable) {
                root.hoverIndex = -1
                return
            }

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

            if (index < 0) {
                root.hoverIndex = -1
                root.clearClicked(root.pressedIndex)
                root.pressedIndex = -1
                mouse.accepted = false
                return
            }

            if (!root.controlsUsable) {
                root.hoverIndex = -1
                root.pressedIndex = -1
                return
            }

            root.hoverIndex = index
            root.pressedIndex = index
            root.setClicked(index)

            SVState.changeEuler(root.pressedIndex % 4, root.pressedIndex >= 4)
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
