import QtQuick
import QtQuick.Shapes 2.15
import QGroundControl

Item {
    id: root

    QGCPalette { id: qgcPalette}

    property int hoverIndex: -1
    property int pressedIndex: -1

    property bool zoomInPressed: false
    property bool zoomOutPressed: false

    function getButton(mouseX, mouseY) {
        if (mouseX < 0 || mouseX > width || mouseY < 0 || mouseY > height) {
            return -1
        }

        return (mouseY > height / 2) ? 1 : 0
    }


    function setPressed(index, pressed) {
        if (index === 0) {
            zoomInPressed = pressed
        } else if (index === 1) {
            zoomOutPressed = pressed
        }
    }

    function clearPressed() {
        zoomInPressed = false
        zoomOutPressed = false
        pressedIndex = -1
    }

    SVZoomButton {
        id: zoomIn
        width: parent.width
        height: parent.height / 2
        anchors.top: parent.top
        buttonColor: {
            if(zoomInPressed) {
                return qgcPalette.buttonHighlight
            } else if(hoverIndex === 0) {
                return qgcPalette.windowShadeLight
            } else {
                return qgcPalette.windowShade
            }
        }
        rotation: 180

        buttonText: "+"
        textColor: qgcPalette.statusPassedText

    }

     SVZoomButton {
        id: zoomOut
        width: parent.width
        height: parent.height / 2
        anchors.bottom: parent.bottom
        buttonColor: {
            if(zoomOutPressed) {
                return qgcPalette.buttonHighlight
            } else if(hoverIndex === 1) {
                return qgcPalette.windowShadeLight
            } else {
                return qgcPalette.windowShade
            }
        }

        buttonText: "-"
        textColor: qgcPalette.statusPassedText

    }

    Rectangle {
        id: innerBorder
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: 1
        color: qgcPalette.windowShadeLight
    }

    Rectangle {
        id: outerBorder
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 1
        border.color: qgcPalette.statusPassedText
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 999

        onPositionChanged: (mouse) => {
            const index = root.getButton(mouse.x, mouse.y)
            root.hoverIndex = index

            if (pressed && index !== root.pressedIndex) {
                root.setPressed(root.pressedIndex, false)
                root.setPressed(index, true)
                root.pressedIndex = index
            }
        }

        onExited: {
            root.hoverIndex = -1
            root.clearPressed()
        }

        onPressed: (mouse) => {
            const index = root.getButton(mouse.x, mouse.y)
            root.hoverIndex = index
            root.pressedIndex = index
            root.setPressed(index, true)
        }

        onReleased: {
            root.clearPressed()
        }

        onCanceled: {
            root.hoverIndex = -1
            root.clearPressed()
        }
    }
}