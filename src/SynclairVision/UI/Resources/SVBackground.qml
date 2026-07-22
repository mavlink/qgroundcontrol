import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property color hoverColor: qgcPalette.windowShadeLight
    property color checkedColor: qgcPalette.buttonHighlight
    property color pressedColor: qgcPalette.buttonHighlight
    property color normalColor: transparentBackground ? "transparent" : qgcPalette.windowTransparent
    property color borderColor: "transparent"
    property color frameBorderColor: qgcPalette.windowShade

    property bool enabled: true
    property bool hoverEnabled: false
    property bool hovered: false
    property bool checkable: false
    property bool checked: false
    property bool pressed: false
    property bool transparentBackground: false
    property point hoverPosition: Qt.point(width / 2, height / 2)
    property color hoverGlowColor: "white"
    property real hoverGlowOpacity: 0.15
    property real hoverGlowRadius: Math.max(width, height) * 0.6

    property real radius: 0
    property bool round: false
    property real borderWidth: 0
    property real frameInset: SVUnits.lineWidth
    property real frameBorderWidth: 1

    property real hoverOpacity: (checked) ? 20 : 12

    QGCPalette {
        id: qgcPalette

        colorGroupEnabled: root.enabled
    }

    onRadiusChanged: {
        if (hoverGlow.visible) {
            hoverGlow.requestPaint()
        }

        if (hoverOutline.visible) {
            hoverOutline.requestPaint()
        }
    }

    onHoverPositionChanged: {
        if (hoverGlow.visible) {
            hoverGlow.requestPaint()
        }
    }

    onHoverGlowColorChanged: {
        if (hoverGlow.visible) {
            hoverGlow.requestPaint()
        }
    }

    onHoverGlowOpacityChanged: {
        if (hoverGlow.visible) {
            hoverGlow.requestPaint()
        }
    }

    onHoverGlowRadiusChanged: {
        if (hoverGlow.visible) {
            hoverGlow.requestPaint()
        }
    }

    onHoveredChanged: {
        if (hoverOutline.visible) {
            hoverOutline.requestPaint()
        }
    }

    Rectangle {
        anchors.fill: parent
        //color: root.pressed ? root.pressedColor : (root.checkable && root.checked ? root.checkedColor : (root.hoverEnabled && root.hovered ? root.hoverColor : qgcPalette.windowTransparent))
        
        color: Qt.tint(
            root.checked ? root.pressedColor : (root.hovered ? hoverColor : root.normalColor), 
            (root.pressed ? Qt.rgba(1.0, 1.0, 1.5, 0.20) : Qt.rgba(0.0, 0.0, 0.0, 0.0))
        )
        
        radius: root.radius
        border.width: root.borderWidth
        border.color: root.borderColor

        Rectangle {
            id: backgroundGradient
            anchors.fill: parent
            visible: !SVSettings.simplifiedUserInterface && !root.transparentBackground
            color: qgcPalette.windowTransparent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: qgcPalette.windowShade }
                GradientStop { position: 0.25; color: "transparent" }
                GradientStop { position: 1.0; color: "transparent" }
            }
            radius: root.radius
            opacity: 0.20
        }

        Rectangle {
            id: backgroundGradient2
            anchors.fill: parent
            visible: !SVSettings.simplifiedUserInterface && !root.transparentBackground
            color: qgcPalette.windowTransparent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.75; color: "transparent" }
                GradientStop { position: 1.0; color: qgcPalette.windowShade }
            }
            radius: root.radius
            opacity: 0.20
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: root.frameInset
            color: "transparent"
            border.width: 1//root.frameBorderWidth
            border.color: root.frameBorderColor
            visible: !SVSettings.simplifiedUserInterface && root.enabled && (root.hovered || root.checked) 
            radius: root.radius

            
        }

        Canvas {
            id: hoverGlow

            anchors.fill: parent
            visible: !SVSettings.simplifiedUserInterface && root.enabled && root.hoverEnabled && root.hovered

            onPaint: {
                if (SVSettings.simplifiedUserInterface || !root.enabled || !root.hoverEnabled || !root.hovered) {
                    return
                }

                const context = getContext("2d")
                const glowX = root.hoverPosition.x
                const glowY = root.hoverPosition.y
                const cornerRadius = Math.min(root.radius, Math.min(width, height) / 2)
                const glowRadius = root.hoverGlowRadius

                context.reset()
                context.clearRect(0, 0, width, height)
                context.beginPath()
                context.moveTo(cornerRadius, 0)
                context.lineTo(width - cornerRadius, 0)
                context.quadraticCurveTo(width, 0, width, cornerRadius)
                context.lineTo(width, height - cornerRadius)
                context.quadraticCurveTo(width, height, width - cornerRadius, height)
                context.lineTo(cornerRadius, height)
                context.quadraticCurveTo(0, height, 0, height - cornerRadius)
                context.lineTo(0, cornerRadius)
                context.quadraticCurveTo(0, 0, cornerRadius, 0)
                context.closePath()
                context.clip()

                const gradient = context.createRadialGradient(glowX, glowY, 0, glowX, glowY, glowRadius)
                gradient.addColorStop(0, Qt.rgba(root.hoverGlowColor.r, root.hoverGlowColor.g, root.hoverGlowColor.b, root.hoverGlowOpacity))
                gradient.addColorStop(1, "rgba(255, 255, 255, 0.0)")
                context.fillStyle = gradient
                context.fillRect(0, 0, width, height)
            }

            onVisibleChanged: {
                if (visible) {
                    requestPaint()
                }
            }

            onWidthChanged: {
                if (visible) {
                    requestPaint()
                }
            }

            onHeightChanged: {
                if (visible) {
                    requestPaint()
                }
            }
        }

        Canvas {
            id: hoverOutline

            anchors.fill: parent
            visible: !SVSettings.simplifiedUserInterface && root.enabled
            opacity: root.hovered ? 1.0 : 0.0

            Behavior on opacity {
                NumberAnimation { duration: 175 }
            }

            onPaint: {
                if (SVSettings.simplifiedUserInterface) {
                    return
                }

                const context = getContext("2d")
                const cornerRadius = Math.min(root.radius, Math.min(width, height) / 2)
                const outlineInset = 0.5
                const outlineRadius = Math.max(0, cornerRadius - outlineInset)

                context.reset()
                context.clearRect(0, 0, width, height)
                context.beginPath()
                context.moveTo(cornerRadius, 0)
                context.lineTo(width - cornerRadius, 0)
                context.quadraticCurveTo(width, 0, width, cornerRadius)
                context.lineTo(width, height - cornerRadius)
                context.quadraticCurveTo(width, height, width - cornerRadius, height)
                context.lineTo(cornerRadius, height)
                context.quadraticCurveTo(0, height, 0, height - cornerRadius)
                context.lineTo(0, cornerRadius)
                context.quadraticCurveTo(0, 0, cornerRadius, 0)
                context.closePath()
                context.clip()

                const outlineGradient = context.createLinearGradient(0, 0, width, height)
                outlineGradient.addColorStop(0, "rgba(255, 255, 255, 0.40)")
                outlineGradient.addColorStop(0.3, "rgba(255, 255, 255, 0.12)")
                outlineGradient.addColorStop(0.5, "rgba(255, 255, 255, 0.0)")
                outlineGradient.addColorStop(0.7, "rgba(255, 255, 255, 0.12)")
                outlineGradient.addColorStop(1, "rgba(255, 255, 255, 0.40)")
                context.strokeStyle = outlineGradient
                context.lineWidth = 1
                context.beginPath()
                context.moveTo(outlineRadius + outlineInset, outlineInset)
                context.lineTo(width - outlineRadius - outlineInset, outlineInset)
                context.quadraticCurveTo(width - outlineInset, outlineInset, width - outlineInset, outlineRadius + outlineInset)
                context.lineTo(width - outlineInset, height - outlineRadius - outlineInset)
                context.quadraticCurveTo(width - outlineInset, height - outlineInset, width - outlineRadius - outlineInset, height - outlineInset)
                context.lineTo(outlineRadius + outlineInset, height - outlineInset)
                context.quadraticCurveTo(outlineInset, height - outlineInset, outlineInset, height - outlineRadius - outlineInset)
                context.lineTo(outlineInset, outlineRadius + outlineInset)
                context.quadraticCurveTo(outlineInset, outlineInset, outlineRadius + outlineInset, outlineInset)
                context.closePath()
                context.stroke()
            }

            onVisibleChanged: {
                if (visible) {
                    requestPaint()
                }
            }

            onWidthChanged: {
                if (visible) {
                    requestPaint()
                }
            }

            onHeightChanged: {
                if (visible) {
                    requestPaint()
                }
            }
        }
    }

    DeadMouseArea {
        anchors.fill: parent
    }

}
