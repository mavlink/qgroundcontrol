import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property real knobSize: 0.3
    property real knobOffsetX: 0
    property real knobOffsetY: 0
    property real joystickRadius: Math.min(background.width, background.height) / 2
    property real knobRadius: Math.max(joystickKnob.width, joystickKnob.height) / 2
    property real movementRadius: Math.max(0, joystickRadius - knobRadius)

    QGCPalette { id: qgcPalette}

    function centerKnob() {
        knobOffsetX = 0
        knobOffsetY = 0
    }

    function updateKnobPosition(pointerX, pointerY) {
        const deltaX = pointerX - (width / 2)
        const deltaY = pointerY - (height / 2)
        const distance = Math.hypot(deltaX, deltaY)

        if (distance <= movementRadius || distance === 0) {
            knobOffsetX = deltaX
            knobOffsetY = deltaY
            return
        }

        const scale = movementRadius / distance
        knobOffsetX = deltaX * scale
        knobOffsetY = deltaY * scale
    }

    function isInsideJoystick(pointerX, pointerY) {
        return Math.hypot(pointerX - (width / 2), pointerY - (height / 2)) <= joystickRadius
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: qgcPalette.windowShade
        border.width: 1
        border.color: qgcPalette.statusPassedText
        radius: width / 2
    }

    Repeater {
        id: arrows

        model: 4

        delegate : Item {
            id: arrow
            required property int index
            rotation: index * 90
            anchors.fill: parent

            SVArrow {
                width: 10
                height: 13
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10

                arrowFilled: true
                outerBorderColor: qgcPalette.window


            }
        }
    }

    Rectangle {
        id: joystickKnob
        width: parent.width * root.knobSize
        height: parent.height * root.knobSize
        x: (root.width / 2) - (width / 2) + root.knobOffsetX
        y: (root.height / 2) - (height / 2) + root.knobOffsetY
        color: qgcPalette.window
        border.width: 1
        border.color: qgcPalette.statusPassedText
        radius: width / 2
    }

    MouseArea {
        anchors.fill: parent

        onPressed: (mouse) => {
            if (!root.isInsideJoystick(mouse.x, mouse.y)) {
                mouse.accepted = false
                return
            }

            root.updateKnobPosition(mouse.x, mouse.y)
        }

        onPositionChanged: (mouse) => {
            if (!pressed) {
                return
            }

            root.updateKnobPosition(mouse.x, mouse.y)
        }

        onReleased: root.centerKnob()
        onCanceled: root.centerKnob()
    }











}
