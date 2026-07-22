import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property real knobSize: SVSettings.joystickKnobSize
    property real knobOffsetX: 0
    property real knobOffsetY: 0

    readonly property real centerX: width / 2
    readonly property real centerY: height / 2
    readonly property real joystickRadius: Math.min(width, height) / 2
    readonly property real knobRadius: Math.max(joystickKnob.width, joystickKnob.height) / 2
    readonly property real movementRadius: Math.max(0, joystickRadius - knobRadius)
    readonly property real deadzoneSize: Math.max(0, Math.min(1, SVSettings.joystickDeadzone))

    readonly property real arrowWidth: width * 0.08
    readonly property real arrowHeight: height * 0.10
    readonly property bool controlsLocked: SVState.lockControls
    readonly property bool controlsUsable: !SVState.lockControls && SVState.cameraSelected !== -1
    readonly property color activeBorderColor: qgcPalette.statusPassedText
    readonly property color disabledArrowColor: qgcPalette.windowShadeLight
    readonly property color arrowColor: root.controlsUsable ? "white" : root.disabledArrowColor
    readonly property real arrowOpacity: root.controlsUsable ? 1 : 0.45
    readonly property color outerRingColor: root.controlsLocked ? qgcPalette.colorRed : root.activeBorderColor
    readonly property color knobBorderColor: root.controlsUsable ? root.activeBorderColor : root.disabledArrowColor

    QGCPalette { id: qgcPalette }

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property real eulerScale: 0.10



    function centerKnob() {
        knobOffsetX = 0
        knobOffsetY = 0
        sendEulerCommand(0, 0)
    }

    function sendEulerCommand(yaw, pitch) {
        if (SVState.cameraSelected < 0) {
            return
        }

        digiview.changeEuler(SVState.cameraSelected, yaw, pitch)
    }

    function updateEulerCommand() {
        if (movementRadius <= 0 || deadzoneSize >= 1) {
            sendEulerCommand(0, 0)
            return
        }

        let yaw = knobOffsetX / movementRadius
        let pitch = knobOffsetY / movementRadius
        const magnitude = Math.hypot(yaw, pitch)

        if (magnitude <= deadzoneSize) {
            sendEulerCommand(0, 0)
            return
        }

        const scaledMagnitude = (magnitude - deadzoneSize) / (1 - deadzoneSize)
        const scale = scaledMagnitude / magnitude

        yaw = -yaw * scale
        pitch = -pitch * scale

        if (SVSettings.joystickInvertHorizontal) {
            yaw = -yaw
        }

        if (SVSettings.joystickInvertVertical) {
            pitch = -pitch
        }

        const sensitivity = SVSettings.joystickSensitivity * eulerScale
        sendEulerCommand(yaw * sensitivity, pitch * sensitivity)
    }

    function updateKnobPosition(pointerX, pointerY) {
        const deltaX = pointerX - centerX
        const deltaY = pointerY - centerY
        const distance = Math.hypot(deltaX, deltaY)

        if (distance === 0 || distance <= movementRadius) {
            knobOffsetX = deltaX
            knobOffsetY = deltaY
            updateEulerCommand()
            return
        }

        const scale = movementRadius / distance
        knobOffsetX = deltaX * scale
        knobOffsetY = deltaY * scale
        updateEulerCommand()
    }

    function isInsideJoystick(pointerX, pointerY) {
        return Math.hypot(pointerX - centerX, pointerY - centerY) <= joystickRadius
    }

    SVBackground {
        id: background2
        anchors.fill: parent
        normalColor: qgcPalette.windowShade
        round: true
        radius: width / 2
        checked: false
        pressed: false
        checkable: false
        hoverEnabled: true
        hovered: dragMouseArea.containsMouse && root.controlsUsable
        hoverPosition: Qt.point(dragMouseArea.mouseX, dragMouseArea.mouseY)
        hoverGlowOpacity: 0.06
        hoverGlowRadius: Math.max(width, height) * 0.65
    }

    
    

    Rectangle {
        id: background
        anchors.fill: parent
        color: qgcPalette.windowShade
        radius: width / 2
        visible: false
    }

    Rectangle {
        id: deadzone
        width: root.movementRadius * 2 * root.deadzoneSize
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: qgcPalette.window
        radius: width / 2
        opacity: 0.3
        visible: false
    }

    Rectangle {
        id: centerPoint
        width: SVUnits.width
        height: width
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        radius: width / 2
        color: root.arrowColor
        opacity: 0.3
    }

    Repeater {
        model: 4

        delegate: Item {
            required property int index

            anchors.fill: parent
            rotation: index * 90

            SVArrow {
                width: root.arrowWidth
                height: root.arrowHeight
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: SVUnits.margin

                arrowFilled: true
                outerBorderColor: root.arrowColor
                opacity: root.arrowOpacity
            }
        }
    }

    Rectangle {
        id: joystickKnob
        width: root.width * root.knobSize
        height: root.height * root.knobSize
        x: root.centerX - (width / 2) + root.knobOffsetX
        y: root.centerY - (height / 2) + root.knobOffsetY
        color: qgcPalette.window
        border.width: SVUnits.lineWidth
        border.color: root.knobBorderColor
        radius: width / 2
    }

    MouseArea {
        id: dragMouseArea
        anchors.fill: parent
        hoverEnabled: true

        onPressed: (mouse) => {
            const isInsideJoystick = root.isInsideJoystick(mouse.x, mouse.y)

            mouse.accepted = isInsideJoystick

            if (!isInsideJoystick) {
                return
            }

            if (!root.controlsUsable) {
                return
            }

            root.updateKnobPosition(mouse.x, mouse.y)
        }

        onPositionChanged: (mouse) => {
            if (pressed && root.controlsUsable) {
                root.updateKnobPosition(mouse.x, mouse.y)
            }
        }

        onReleased: root.centerKnob()
        onCanceled: root.centerKnob()
    }
}
