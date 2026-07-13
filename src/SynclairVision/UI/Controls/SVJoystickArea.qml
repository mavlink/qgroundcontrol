import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls


Item {
    id: root

    property real t: SVSettings.joystickRatio
    property int hoverIndex: -1
    property int pressedIndex: -1

    property var  innerClicked: [false, false, false, false]
    property var  outerClicked: [false, false, false, false]
    property var  hasInnerRing: true

    readonly property bool controlsLocked: SVState.lockControls
    readonly property bool controlsUsable: !SVState.lockControls && SVState.cameraSelected !== -1
    readonly property color activeBorderColor: qgcPalette.statusPassedText
    readonly property color disabledArrowColor: qgcPalette.windowShadeLight
    readonly property color arrowColor: root.controlsUsable ? "white" : root.disabledArrowColor
    readonly property real arrowOpacity: root.controlsUsable ? 1 : 0.45
    readonly property color outerBorderColor: root.activeBorderColor//root.controlsLocked ? qgcPalette.colorRed : root.activeBorderColor
    readonly property color innerBorderColor: !root.controlsUsable ? qgcPalette.windowShadeLight : root.activeBorderColor

    QGCPalette { id: qgcPalette}

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property real eulerScale: 0.50

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

    function changeEuler() {
        if (root.pressedIndex < 0) {
            return
        }

        let yaw = 0
        let pitch = 0
        let direction = root.pressedIndex % 4

        let strength = SVSettings.joystickSensitivity * eulerScale
        if (root.pressedIndex >= 4) {
            strength *= 0.333
        }

        switch (direction) {
        case 0:
            yaw = -strength
            break
        case 1:
            pitch = -strength
            break
        case 2:
            yaw = strength
            break
        case 3:
            pitch = strength
            break
        }

        if (SVSettings.invertJoystickX) {
            yaw = -yaw
        }

        if (SVSettings.invertJoystickY) {
            pitch = -pitch
        }

        digiview.changeEuler(SVState.cameraSelected, yaw, pitch)
    }

    SVJoystickButtonSegment {
        id: outerButtons
        anchors.fill: parent
        buttonColor: qgcPalette.windowShade
        hoveredButtonColor: qgcPalette.windowShadeLight
        clickedButtonColor: qgcPalette.buttonHighlight
        borderColor: qgcPalette.windowShadeLight
        outerBorderColor: root.outerBorderColor
        hoverIndex: (root.hoverIndex >= 0 && root.hoverIndex < 4) ? root.hoverIndex : -1
        clicked: outerClicked
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
        buttonColor: qgcPalette.window
        hoveredButtonColor: qgcPalette.windowShadeLight
        clickedButtonColor: qgcPalette.buttonHighlight
        borderColor: qgcPalette.windowShadeLight
        outerBorderColor: innerBorderColor
        hoverIndex: (root.hoverIndex >= 4) ? root.hoverIndex - 4 : -1

        clicked: innerClicked
        arrowSize: 0.3
        arrowSpace: 0.8
        arrowColor: root.arrowColor
        arrowOpacity: root.arrowOpacity

        visible: hasInnerRing
    }

    MouseArea {
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

            changeEuler();
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
