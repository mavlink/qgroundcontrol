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

    // Mode-inställningar:
    // 0 = Click (Endast vid nedtryckning)
    // 1 = Press (Kör kontinuerligt direkt)
    // 2 = Click + Press (Klickar direkt, väntar fördröjning, kör sedan kontinuerligt)
    property int inputMode: SVSettings.controlPanelInteraction
    property int holdDelay: 400       // Fördröjning i ms innan upprepning startar (för läge 2)
    property int repeatInterval: 20  // Hastighet i ms mellan varje steg

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

    // --- TIMER-LOGIK FÖR DE 3 LÄGENA ---
    function triggerAction(strength) {
        if (root.pressedIndex >= 0 && root.controlsUsable) {
            SVState.changeEuler(root.pressedIndex % 4, root.pressedIndex >= 4, SVSettings.joystickSensitivity * strength)
        }
    }

    Timer {
        id: holdDelayTimer
        interval: root.holdDelay
        repeat: false
        onTriggered: repeatTimer.restart()
    }

    Timer {
        id: repeatTimer
        interval: root.repeatInterval
        repeat: true
        onTriggered: root.triggerAction(0.1)
    }

    function stopTimers() {
        holdDelayTimer.stop()
        repeatTimer.stop()
    }

    function startInputModeLogic() {
        stopTimers()
        if (!root.controlsUsable || root.pressedIndex < 0) return

        if (root.inputMode === 0) {
            // Mode 0: Click (Bara en gång)
            root.triggerAction(0.5)
        } else if (root.inputMode === 1) {
            // Mode 1: Press (Direkt kontinuerlig)
            root.triggerAction(0.5)
            repeatTimer.restart()
        } else if (root.inputMode === 2) {
            // Mode 2: Click + Press (Klicka direkt, vänta delay, sedan kontinuerlig)
            root.triggerAction(0.5)
            holdDelayTimer.restart()
        }
    }
    // ----------------------------------

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

        property int initialPressedIndex: -1

        onPositionChanged: (mouse) => {
            const index = root.getHoveredButton(mouse.x, mouse.y)

            if (!root.controlsUsable) {
                root.hoverIndex = -1
                root.stopTimers()
                return
            }

            root.hoverIndex = index

            if (pressed) {
                if (root.inputMode === 1) {
                    // Mode 1 (Press): Allow dragging between different buttons seamlessly
                    if (index !== root.pressedIndex) {
                        if (root.pressedIndex !== -1) {
                            root.clearClicked(root.pressedIndex)
                        }
                        
                        root.pressedIndex = index
                        
                        if (index >= 0) {
                            root.setClicked(index)
                            root.startInputModeLogic()
                        } else {
                            root.stopTimers()
                        }
                    }
                } else {
                    // Mode 0 & 2 (Click / Click + Press): Lock strictly to the initially clicked button
                    if (index === initialPressedIndex && initialPressedIndex >= 0) {
                        if (root.pressedIndex !== initialPressedIndex) {
                            root.pressedIndex = initialPressedIndex
                            root.setClicked(initialPressedIndex)
                            root.startInputModeLogic()
                        }
                    } else {
                        if (root.pressedIndex !== -1) {
                            root.clearClicked(root.pressedIndex)
                            root.pressedIndex = -1
                            root.stopTimers()
                        }
                    }
                }
            }
        }

        onExited: {
            root.stopTimers()
            root.hoverIndex = -1
            if (root.pressedIndex !== -1) {
                root.clearClicked(root.pressedIndex)
                root.pressedIndex = -1
            }
        }

        onPressed: (mouse) => {
            const index = root.getHoveredButton(mouse.x, mouse.y)

            if (index < 0) {
                root.stopTimers()
                root.hoverIndex = -1
                if (root.pressedIndex !== -1) {
                    root.clearClicked(root.pressedIndex)
                    root.pressedIndex = -1
                }
                initialPressedIndex = -1
                mouse.accepted = false
                return
            }

            if (!root.controlsUsable) {
                root.stopTimers()
                root.hoverIndex = -1
                if (root.pressedIndex !== -1) {
                    root.clearClicked(root.pressedIndex)
                    root.pressedIndex = -1
                }
                initialPressedIndex = -1
                return
            }

            initialPressedIndex = index
            root.hoverIndex = index
            root.pressedIndex = index
            root.setClicked(index)

            root.startInputModeLogic()
        }

        onReleased: {
            root.stopTimers()
            if (root.pressedIndex !== -1) {
                root.clearClicked(root.pressedIndex)
                root.pressedIndex = -1
            }
            initialPressedIndex = -1
        }

        onCanceled: {
            root.stopTimers()
            if (root.pressedIndex !== -1) {
                root.clearClicked(root.pressedIndex)
                root.pressedIndex = -1
            }
            root.hoverIndex = -1
            initialPressedIndex = -1
        }
    }
}