import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property string text: ""
    property url iconSource
    property bool tintIcon: true
    property bool checked: false
    property bool enabled: true
    property bool extendHeader: false
    property bool expanded: false

    property real size: ScreenTools.defaultFontPixelWidth * 7
    property real borderRadius: ScreenTools.defaultBorderRadius
    property real iconScale: root.text === "" ? 0.6 : 0.45
    property real spacing: ScreenTools.defaultFontPixelHeight * 0.2
    property real contentSize: size - SVUnits.bigMargin * 2

    readonly property color foregroundColor: root.checked ? qgcPalette.buttonHighlightText : qgcPalette.statusFailedText
    readonly property color backgroundColor: (mouseArea.pressed || root.checked)
                                             ? qgcPalette.buttonHighlight
                                             : (mouseArea.containsMouse ? qgcPalette.windowShadeLight : "transparent")
    readonly property real frameInset: ScreenTools.defaultFontPixelWidth * 0.25

    signal clicked

    width: size
    height: size

    onBorderRadiusChanged: {
        if (hoverGlow.visible) {
            hoverGlow.requestPaint()
        }

        if (hoverOutline.visible) {
            hoverOutline.requestPaint()
        }
    }

    QGCPalette { id: qgcPalette; colorGroupEnabled: root.enabled }

    Rectangle {
        id: background
        anchors.fill: parent
       //anchors.margins: SVUnits.lineWidth
        radius: root.borderRadius
        color: root.backgroundColor

        Canvas {
            id: hoverGlow
            anchors.fill: parent
            visible: !SVSettings.simplifiedUserInterface && root.enabled && mouseArea.containsMouse

            onPaint: {
                if (SVSettings.simplifiedUserInterface || !root.enabled || !mouseArea.containsMouse) {
                    return
                }

                const context = getContext("2d")
                const glowX = mouseArea.mouseX - background.x
                const glowY = mouseArea.mouseY - background.y
                const cornerRadius = Math.min(root.borderRadius, Math.min(width, height) / 2)
                const glowRadius = Math.max(width, height) * 0.6

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
                gradient.addColorStop(0, "rgba(255, 255, 255, 0.12)")
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
            opacity: mouseArea.containsMouse ? 1 : 0

            Behavior on opacity {
                NumberAnimation { duration: 125 }
            }

            onPaint: {
                if (SVSettings.simplifiedUserInterface) {
                    return
                }

                const context = getContext("2d")
                const cornerRadius = Math.min(root.borderRadius, Math.min(width, height) / 2)
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

                const outlineGradient = context.createLinearGradient(0, 0, width, 0)
                outlineGradient.addColorStop(0, "rgba(255, 255, 255, 0.33)")
                outlineGradient.addColorStop(0.15, "rgba(255, 255, 255, 0.33)")
                outlineGradient.addColorStop(0.7, "rgba(255, 255, 255, 0.07)")
                outlineGradient.addColorStop(1, "rgba(255, 255, 255, 0.0)")
                context.strokeStyle = outlineGradient
                context.lineWidth = 1
                context.beginPath()
                context.moveTo(width, outlineInset)
                context.lineTo(outlineRadius + outlineInset, outlineInset)
                context.quadraticCurveTo(outlineInset, outlineInset, outlineInset, outlineRadius + outlineInset)
                context.lineTo(outlineInset, height - outlineRadius - outlineInset)
                context.quadraticCurveTo(outlineInset, height - outlineInset, outlineRadius + outlineInset, height - outlineInset)
                context.lineTo(width, height - outlineInset)
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

    Rectangle {
        anchors.fill: parent
        anchors.margins: SVUnits.lineWidth
        color: "transparent"
        border.width: 1
        border.color: qgcPalette.windowShade
        visible: root.checked && !SVSettings.simplifiedUserInterface
        radius: root.borderRadius
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onClicked: root.clicked()
        onContainsMouseChanged: {
            if (hoverOutline.visible) {
                hoverOutline.requestPaint()
            }
        }
        onPositionChanged: {
            if (hoverGlow.visible) {
                hoverGlow.requestPaint()
            }
        }
    }

    SVArrow {
        id: arrow
        visible: root.extendHeader
        width: ScreenTools.defaultFontPixelWidth * 0.6
        height: width * 1.3
        anchors.top: parent.top
        anchors.topMargin: root.height / 2 - root.spacing * 2.5
        anchors.right: parent.right
        anchors.rightMargin: root.spacing * 2
        arrowFilled: true
        outerBorderColor: root.foregroundColor

        rotation: root.expanded ? 90 : 270

        Behavior on rotation {
            NumberAnimation { duration: 60 }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: root.spacing / 2

        Item {
            width: root.contentSize * root.iconScale
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.iconSource !== ""

            QGCColoredImage {
                anchors.fill: parent
                source: root.iconSource
                color: root.foregroundColor
                visible: root.tintIcon
            }

            Image {
                anchors.fill: parent
                source: root.iconSource
                smooth: true
                mipmap: true
                antialiasing: true
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                sourceSize.height: height
                visible: !root.tintIcon
            }
        }

        QGCLabel {
            text: root.text
            font.pointSize: ScreenTools.smallFontPointSize
            color: root.foregroundColor
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
