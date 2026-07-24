import QtQuick
import QtQuick.Shapes 2.15
import QGroundControl

Item {
    id: root

    QGCPalette { id: qgcPalette }

    property int hoverIndex: -1
    readonly property bool controlsUsable: !SVState.lockControls && SVState.cameraSelected !== -1

    property bool zoomInPressed: false
    property bool zoomOutPressed: false
    readonly property bool zoomInVisualPressed: zoomInPressed || SVState.shortcutZoomInHeld
    readonly property bool zoomOutVisualPressed: zoomOutPressed || SVState.shortcutZoomOutHeld

    property color textColor: root.controlsUsable
        ? qgcPalette.statusPassedText
        : qgcPalette.windowShadeLight

    property color borderColor: qgcPalette.statusPassedText

    function buttonAt(x, y) {
        if (x < 0 || x > width || y < 0 || y > height) {
            return -1
        }

        return y < height / 2 ? 0 : 1
    }

    function setPressed(index, pressed) {
        if (index === 0) {
            zoomInPressed = pressed
        } else if (index === 1) {
            zoomOutPressed = pressed
        }
    }

    function buttonColor(index, pressed) {
        if (pressed) {
            return qgcPalette.buttonHighlight
        }

        if (hoverIndex === index) {
            return SVSettings.simplifiedUserInterface ? qgcPalette.windowShadeLight : Qt.alpha(qgcPalette.windowShadeLight, 0.8)
        }

        return SVSettings.simplifiedUserInterface ? qgcPalette.windowShade : qgcPalette.windowTransparent
    }

    function clearMouseState() {
        hoverIndex = -1
    }

    function clearPressedState() {
        zoomInPressed = false
        zoomOutPressed = false
    }

    function changeZoom() {
        let zoom = 0

        if (zoomInPressed) {
            zoom = SVSettings.zoomSensitivity
        } else if (zoomOutPressed) {
            zoom = -SVSettings.zoomSensitivity
        }

        SVState.changeZoom(zoom)
    }

    SVZoomButton {
        id: zoomIn
        width: parent.width
        height: parent.height / 2
        anchors.top: parent.top
        text: "+"
        buttonColor: root.buttonColor(0, root.zoomInVisualPressed)
        textColor: root.textColor
        rotation: 180
    }

    SVZoomButton {
        id: zoomOut
        width: parent.width
        height: parent.height / 2
        anchors.bottom: parent.bottom
        text: "-"
        buttonColor: root.buttonColor(1, root.zoomOutVisualPressed)
        textColor: root.textColor
    }

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: SVUnits.lineWidth
        color: (hoverIndex === -1 && !zoomInPressed && !zoomOutPressed && !zoomInVisualPressed && !zoomOutVisualPressed) 
        ? qgcPalette.windowShadeLight 
        : "white"
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: SVUnits.lineWidth
        border.color: root.borderColor
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 999

        property int pressedButtonIndex: -1

        onPositionChanged: (mouse) => {
            const index = root.buttonAt(mouse.x, mouse.y)

            if (!root.controlsUsable) {
                root.hoverIndex = -1
                return
            }

            root.hoverIndex = index

            if (pressed && index !== pressedButtonIndex) {
                root.setPressed(pressedButtonIndex, false)
                root.setPressed(index, true)
                pressedButtonIndex = index
            }
        }

        onPressed: (mouse) => {
            const index = root.buttonAt(mouse.x, mouse.y)

            if (index < 0 && !root.controlsUsable) {
                mouse.accepted = false
                return
            }

            if (!root.controlsUsable) {
                root.hoverIndex = -1
                pressedButtonIndex = -1
                return
            }

            root.hoverIndex = index
            pressedButtonIndex = index
            root.setPressed(index, true)

            changeZoom()
            
        }

        onReleased: {
            root.setPressed(pressedButtonIndex, false)
            pressedButtonIndex = -1
        }

        onExited: {
            root.clearMouseState()
        }

        onCanceled: {
            root.setPressed(pressedButtonIndex, false)
            pressedButtonIndex = -1
            root.clearMouseState()
        }
    }
}
