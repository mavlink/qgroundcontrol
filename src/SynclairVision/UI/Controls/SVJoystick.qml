import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    property string joystickType: "standard"
    readonly property bool controlsLocked: SVState.lockControls
    readonly property color lockOverlayColor: qgcPalette.colorRed
    readonly property real lockOverlayOpacity: 0.5
    readonly property url lockOverlaySource: Qt.resolvedUrl("../Resources/Images/halftone_texture_2.png")

    onLockOverlayColorChanged: lockOverlay.requestPaint()

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: border
        anchors.fill: parent
        radius: width / 2
        color: qgcPalette.window
    }

    Loader {
        id: joystickLoader
        anchors.fill: parent
        anchors.margins: SVUnits.margin

        sourceComponent: {
            if(root.joystickType === "standard") {
                return standardComponent
            } else if(root.joystickType === "simple") {
                return simpleComponent
            } else if(root.joystickType === "drag") {
                return dragComponent
            }
        }
    }

    Image {
        id: lockOverlayTexture
        visible: false
        source: root.lockOverlaySource

        onStatusChanged: {
            if (status === Image.Ready) {
                lockOverlay.requestPaint()
            }
        }
    }

    Canvas {
        id: lockOverlay
        anchors.fill: parent
        anchors.margins: SVUnits.margin
        visible: root.controlsLocked
        z: 1
        renderTarget: Canvas.Image

        readonly property int overlayRed: Math.round(root.lockOverlayColor.r * 255)
        readonly property int overlayGreen: Math.round(root.lockOverlayColor.g * 255)
        readonly property int overlayBlue: Math.round(root.lockOverlayColor.b * 255)

        onPaint: {
            const context = getContext('2d')
            context.clearRect(0, 0, width, height)

            if (!visible || lockOverlayTexture.status !== Image.Ready) {
                return
            }

            context.save()
            context.beginPath()
            context.arc(width / 2, height / 2, Math.min(width, height) / 2, 0, Math.PI * 2)
            context.closePath()
            context.clip()
            context.drawImage(lockOverlayTexture, 0, 0, width, height)

            let imageData = context.getImageData(0, 0, width, height)
            let imageBytes = imageData.data

            for (let byteIndex = 0; byteIndex < imageBytes.length; byteIndex += 4) {
                const originalAlpha = imageBytes[byteIndex + 3] / 255
                const brightness = Math.max(imageBytes[byteIndex], imageBytes[byteIndex + 1], imageBytes[byteIndex + 2]) / 255
                const stripeOpacity = (1 - brightness) * originalAlpha * root.lockOverlayOpacity

                imageBytes[byteIndex] = overlayRed
                imageBytes[byteIndex + 1] = overlayGreen
                imageBytes[byteIndex + 2] = overlayBlue
                imageBytes[byteIndex + 3] = Math.round(stripeOpacity * 255)
            }

            context.putImageData(imageData, 0, 0)
            context.restore()
        }

        onVisibleChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Component {
        id: standardComponent
        SVJoystickArea {
            hasInnerRing: true
        }
    }

    Component {
        id: simpleComponent
        SVJoystickArea {
            hasInnerRing: false
        }
    }

    Component {
        id: dragComponent
        SVJoystickDrag {}
    }
}
